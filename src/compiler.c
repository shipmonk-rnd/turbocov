#include "turbocov_internal.h"

int tc_slot = -1;
bool tc_installed;
HashTable tc_files;
HashTable tc_units;
zend_ulong tc_stat_probes;
zend_ulong tc_stat_jump_table_files;

static zend_ulong tc_ignored_opcodes[256 / ZEND_BITSET_ELM_SIZE];

/* Opcode => the operand (1 or 2) that makes the opcode a consumer when it is a temporary value, or 0. */
static uint8_t tc_consumer_operand[256];

static zend_op_array *(*tc_orig_compile_file)(zend_file_handle *file_handle, int type);
static zend_op_array *(*tc_orig_compile_string)(zend_string *source_string, const char *filename, zend_compile_position position);

void tc_fill_ignored_opcodes(void)
{
	/* The list of PCOV, so that both drivers report the same executable lines. */
	static const uint8_t ignored[] = {
		ZEND_NOP, ZEND_OP_DATA, ZEND_FE_FREE, ZEND_FREE, ZEND_ASSERT_CHECK, ZEND_VERIFY_RETURN_TYPE,
		ZEND_RECV, ZEND_RECV_INIT, ZEND_RECV_VARIADIC, ZEND_SEND_VAL, ZEND_SEND_VAR_EX, ZEND_SEND_REF,
		ZEND_SEND_UNPACK, ZEND_DECLARE_CONST, ZEND_DECLARE_CLASS, ZEND_DECLARE_CLASS_DELAYED,
		ZEND_DECLARE_FUNCTION, ZEND_DECLARE_ANON_CLASS, ZEND_FAST_RET, ZEND_FAST_CALL, ZEND_TICKS,
		ZEND_EXT_STMT, ZEND_EXT_FCALL_BEGIN, ZEND_EXT_FCALL_END, ZEND_EXT_NOP, ZEND_BIND_GLOBAL,
	};

	/*
	 * These opcodes only use the value of the expression that PHP compiled before them. Their own token has no operand after that expression.
	 * An opcode with an operand after the expression must stay out: IS_SMALLER has its operands in reverse order for ">", FE_FETCH has "as $value".
	 * MUL, the bitwise opcodes and the equality opcodes must stay out too: PHP swaps their operands when the first one is a temporary value
	 * and the second one is a variable. Then "(expression) === $variable" looks the same as "$variable === (expression)".
	 */
	static const uint8_t first_operand[] = {
		ZEND_JMPZ, ZEND_JMPNZ, ZEND_JMPZ_EX, ZEND_JMPNZ_EX, ZEND_JMP_SET, ZEND_COALESCE, ZEND_JMP_NULL,
		ZEND_BW_NOT, ZEND_CAST, ZEND_QM_ASSIGN, ZEND_ECHO, ZEND_STRLEN, ZEND_COUNT,
#if PHP_VERSION_ID >= 80500
		/* Before PHP 8.5 the compiler also makes BOOL from "X == true" and BOOL_NOT from "X == false", where the token is after the expression. */
		ZEND_BOOL, ZEND_BOOL_NOT,
#endif
		ZEND_RETURN, ZEND_RETURN_BY_REF, ZEND_GENERATOR_RETURN,
		ZEND_INIT_ARRAY, ZEND_ADD_ARRAY_ELEMENT, ZEND_ADD_ARRAY_UNPACK, ZEND_FETCH_LIST_R,
		ZEND_SEND_VAL_EX, ZEND_SEND_VAR_NO_REF, ZEND_SEND_VAR_NO_REF_EX,
	};
	static const uint8_t second_operand[] = {
		ZEND_ADD, ZEND_SUB, ZEND_DIV, ZEND_MOD, ZEND_SL, ZEND_SR, ZEND_CONCAT, ZEND_POW,
	};

	for (size_t i = 0; i < sizeof(ignored); i++) {
		zend_bitset_incl(tc_ignored_opcodes, ignored[i]);
	}

	for (size_t i = 0; i < sizeof(first_operand); i++) {
		tc_consumer_operand[first_operand[i]] = 1;
	}

	for (size_t i = 0; i < sizeof(second_operand); i++) {
		tc_consumer_operand[second_operand[i]] = 2;
	}
}

/* Returns the record of a whitelisted file, or NULL. */
static tc_file *tc_file_for(zend_string *filename)
{
	zval *known;
	zval entry;
	tc_file *file = NULL;

	if (!filename) {
		return NULL;
	}

	known = zend_hash_find(&tc_files, filename);

	if (known) {
		return Z_PTR_P(known);
	}

	if (tc_paths_select(filename)) {
		file = pecalloc(1, sizeof(tc_file), 1);
		file->name = zend_string_copy(filename);
		zend_hash_init(&file->lines, 64, NULL, NULL, 1);
	}

	/* zend_hash_add_ptr() does not accept NULL, which is the value for a file that is not selected. */
	ZVAL_PTR(&entry, file);
	zend_hash_add(&tc_files, filename, &entry);

	return file;
}

static void tc_files_dtor(zval *zv)
{
	tc_file *file = Z_PTR_P(zv);

	if (file) {
		zend_string_release(file->name);
		zend_hash_destroy(&file->lines);
		pefree(file, 1);
	}
}

/*
 * PHP gives an opcode that only uses the value of an expression the line of the last operand of that expression.
 * The jumps that skip this operand go to that opcode. Example: the JMPZ of "if ($a\n || $b)" has the line of $b, and it runs also when $a is true.
 * Such opcodes get no probe in the block after the operand. The operand has its own probe, and it always runs before control falls into that block.
 * This function returns the line of the operand: the last line of the block before, when that block continues in this block.
 */
static uint32_t tc_operand_line(const zend_cfg *cfg, int number, uint32_t last_line)
{
	const zend_basic_block *block = &cfg->blocks[number];

	if (last_line == 0 || (block->flags & (ZEND_BB_START | ZEND_BB_RECV_ENTRY | ZEND_BB_CATCH | ZEND_BB_FINALLY | ZEND_BB_FINALLY_END))) {
		return 0;
	}

	/* The type of successors_count is int before PHP 8.6 and uint32_t since PHP 8.6. */
	for (uint32_t i = 0; i < (uint32_t) (block - 1)->successors_count; i++) {
		if ((block - 1)->successors[i] == number) {
			return last_line;
		}
	}

	return 0;
}

static bool tc_sends_argument(uint8_t opcode)
{
	switch (opcode) {
		case ZEND_SEND_VAL:
		case ZEND_SEND_VAL_EX:
		case ZEND_SEND_VAR:
		case ZEND_SEND_VAR_EX:
		case ZEND_SEND_REF:
		case ZEND_SEND_VAR_NO_REF:
		case ZEND_SEND_VAR_NO_REF_EX:
		case ZEND_SEND_FUNC_ARG:
		case ZEND_SEND_UNPACK:
		case ZEND_SEND_ARRAY:
		case ZEND_SEND_USER:
			return true;
		default:
			return false;
	}
}

/* Checks the opcodes from the given one to the next opcode of a different line. */
static bool tc_line_has_only_consumers(const zend_op *opline, const zend_op *end)
{
	uint32_t line = opline->lineno;
	bool open_result = false;

	for (; opline < end; opline++) {
		uint8_t type;

		if (zend_bitset_in(tc_ignored_opcodes, opline->opcode)) {
			/* An ignored opcode with such an operand shows that the line has one more operand, for example a variable that is an argument. */
			if (opline->lineno == line && ((opline->op1_type | opline->op2_type) & (IS_CONST | IS_CV))) {
				return false;
			}

			open_result = false;

			continue;
		}

		if (opline->lineno != line) {
			break;
		}

		if (opline->opcode == ZEND_JMP) {
			/* A value that waits for its consumer shows that this jump is inside an expression. Thus it is not a "break" or a "continue". */
			if (!open_result) {
				return false;
			}

			open_result = false;

			continue;
		}

		type = tc_consumer_operand[opline->opcode] == 1 ? opline->op1_type : (tc_consumer_operand[opline->opcode] == 2 ? opline->op2_type : 0);

		if (!(type & (IS_TMP_VAR | IS_VAR))) {
			return false;
		}

		/* A variable as the next argument is after the expression, and PHP can give its opcode the line of the first argument. */
		if (tc_sends_argument(opline->opcode) && opline + 1 < end && tc_sends_argument((opline + 1)->opcode) && (opline + 1)->op1_type == IS_CV) {
			return false;
		}

		open_result = (opline->result_type & (IS_TMP_VAR | IS_VAR)) != 0;
	}

	return true;
}

static tc_unit *tc_install_probes(zend_op_array *op_array, tc_file *file)
{
	zend_arena *arena = zend_arena_create(64 * 1024);
	zend_cfg cfg;
	zend_op *limit = op_array->opcodes + op_array->last;
	tc_unit *unit = pecalloc(1, sizeof(tc_unit), 1);
	uint32_t last_line = 0;
	zval uncovered;

	ZVAL_LONG(&uncovered, TC_UNCOVERED);

	unit->opcodes = op_array->opcodes;
	unit->file = file;
	unit->last = op_array->last;
	unit->index = pecalloc(op_array->last, sizeof(uint32_t), 1);
	unit->probes = pecalloc(op_array->last, sizeof(tc_probe), 1);

	memset(&cfg, 0, sizeof(zend_cfg));
	zend_build_cfg(&arena, op_array, 0, &cfg);

	/* The type of blocks_count is int before PHP 8.6 and uint32_t since PHP 8.6. */
	for (uint32_t i = 0; i < (uint32_t) cfg.blocks_count; i++) {
		zend_basic_block *block = &cfg.blocks[i];
		zend_op *opline = op_array->opcodes + block->start;
		zend_op *end = opline + block->len;
		bool block_entry = true;
		uint32_t probed_line = 0;
		uint32_t operand_line;
		bool consumers = false;

		/* PCOV does not report unreachable code, and such code cannot run. */
		if (!(block->flags & ZEND_BB_REACHABLE)) {
			last_line = 0;

			continue;
		}

		operand_line = tc_operand_line(&cfg, i, last_line);
		last_line = 0;

		for (; opline < end; opline++) {
			if (zend_bitset_in(tc_ignored_opcodes, opline->opcode)) {
				/* A call that passes N arguments starts after the Nth RECV opcode. */
				if (opline->opcode == ZEND_RECV || opline->opcode == ZEND_RECV_INIT || opline->opcode == ZEND_RECV_VARIADIC) {
					block_entry = true;
				}

				if ((opline->op1_type | opline->op2_type) & (IS_CONST | IS_CV)) {
					operand_line = 0;
				}

				continue;
			}

			zend_hash_index_add(&file->lines, opline->lineno, &uncovered);
			last_line = opline->lineno;

			if (block_entry || opline->lineno != probed_line) {
				consumers = opline->lineno == operand_line && tc_line_has_only_consumers(opline, end);
			}

			/*
			 * A variable or a constant can be an operand on the operand line, and an opcode of a different line can use it.
			 * After such an opcode, the line of a consumer can be the line of that operand.
			 */
			if (!consumers && ((opline->op1_type | opline->op2_type) & (IS_CONST | IS_CV))) {
				operand_line = 0;
			}

			if (consumers) {
				block_entry = false;
				probed_line = opline->lineno;
			} else if (block_entry || opline->lineno != probed_line) {
				tc_probe *probe = &unit->probes[unit->count];

				probe->handler = opline->handler;
				probe->opnum = (uint32_t) (opline - op_array->opcodes);
				probe->line = opline->lineno;
				probe->state = TC_ARMED;

				unit->index[probe->opnum] = ++unit->count;
				opline->handler = tc_probe_label;

				block_entry = false;
				probed_line = opline->lineno;
				tc_stat_probes++;
			}

			/* PCOV does not count the constructor call of a "new" expression as executable. */
			if (opline->opcode == ZEND_NEW && opline + 1 < end && (opline + 1)->opcode == ZEND_DO_FCALL) {
				opline++;
			}
		}

		if (block == cfg.blocks && opline == limit) {
			break;
		}
	}

	zend_arena_destroy(arena);

	return unit;
}

/*
 * pass_two() gave every opcode the ZEND_USER_OPCODE handler, because all user opcode handlers are registered.
 * This function runs while no handler is registered, thus zend_vm_set_opcode_handler() restores the real handlers.
 */
static void tc_process_op_array(zend_op_array *op_array)
{
	tc_file *file;
	tc_unit *unit = NULL;

	if (!ZEND_USER_CODE(op_array->type) || !op_array->opcodes || !(op_array->fn_flags & ZEND_ACC_DONE_PASS_TWO)) {
		return;
	}

	if (op_array->reserved[tc_slot]) {
		return;
	}

	file = tc_file_for(op_array->filename);

	if (file) {
		unit = zend_hash_index_find_ptr(&tc_units, (zend_ulong) (uintptr_t) op_array->opcodes);
	}

	if (!unit) {
		for (uint32_t i = 0; i < op_array->last; i++) {
			zend_vm_set_opcode_handler(&op_array->opcodes[i]);
		}

		if (file && !(op_array->fn_flags & ZEND_ACC_ABSTRACT)) {
			unit = tc_install_probes(op_array, file);
			zend_hash_index_add_ptr(&tc_units, (zend_ulong) (uintptr_t) op_array->opcodes, unit);
		}
	}

	op_array->reserved[tc_slot] = unit ? (void *) unit : TC_NO_PROBES;

	for (uint32_t i = 0; i < op_array->num_dynamic_func_defs; i++) {
		tc_process_op_array(op_array->dynamic_func_defs[i]);
	}
}

/*
 * PHP 8.6 compiles the closure of a partial function application at run time, and the compile hooks do not see it.
 * pass_two() gave each of its opcodes the ZEND_USER_OPCODE handler, thus its first opcode comes to the probe callback.
 */
bool tc_adopt_op_array(zend_op_array *op_array, const zend_op *opline)
{
	if (!tc_active || opline < op_array->opcodes || opline >= op_array->opcodes + op_array->last) {
		return false;
	}

	tc_unregister_handlers();
	tc_process_op_array(op_array);
	tc_register_handlers();

	return true;
}

static void tc_process_class(zend_class_entry *ce)
{
	zend_function *function;

	if (ce->type != ZEND_USER_CLASS) {
		return;
	}

	ZEND_HASH_MAP_FOREACH_PTR(&ce->function_table, function) {
		if (function->type == ZEND_USER_FUNCTION) {
			tc_process_op_array(&function->op_array);
		}
	} ZEND_HASH_FOREACH_END();

	if (ce->num_hooked_props > 0) {
		zend_property_info *property;

		ZEND_HASH_MAP_FOREACH_PTR(&ce->properties_info, property) {
			if (!property->hooks) {
				continue;
			}

			for (uint32_t i = 0; i < ZEND_PROPERTY_HOOK_COUNT; i++) {
				if (property->hooks[i] && property->hooks[i]->type == ZEND_USER_FUNCTION) {
					tc_process_op_array(&property->hooks[i]->op_array);
				}
			}
		} ZEND_HASH_FOREACH_END();
	}
}

static void tc_after_compile(zend_op_array *main_op_array, uint32_t first_function, uint32_t first_class)
{
	zend_function *function;
	zend_class_entry *ce;

	if (!tc_active || !tc_thread_is_owner()) {
		return;
	}

	tc_threads_lock();

	/* A different thread can run an opcode that has the ZEND_USER_OPCODE handler. The table must stay complete for it. */
	if (tc_second_thread) {
		tc_threads_unlock();

		return;
	}

	tc_unregister_handlers();

	if (main_op_array) {
		tc_process_op_array(main_op_array);
	}

	ZEND_HASH_MAP_FOREACH_PTR_FROM(CG(function_table), function, first_function) {
		if (function->type == ZEND_USER_FUNCTION) {
			tc_process_op_array(&function->op_array);
		}
	} ZEND_HASH_FOREACH_END();

	ZEND_HASH_MAP_FOREACH_PTR_FROM(CG(class_table), ce, first_class) {
		tc_process_class(ce);
	} ZEND_HASH_FOREACH_END();

	tc_register_handlers();
	tc_threads_unlock();

	if (main_op_array) {
		tc_record_skipped_return(main_op_array);
	}
}

/*
 * A file that is not selected can keep its jump tables, so its switch and match statements run at full speed.
 * The decision must be made before PHP compiles the file. The name of the compiled file is the resolved path.
 * If the path cannot be resolved, the file has no jump tables: a wrong "not selected" loses lines of the report.
 */
static bool tc_can_keep_jump_tables(zend_file_handle *file_handle)
{
	zend_string *resolved;
	bool selected;

	if (!tc_active || !tc_thread_is_owner()) {
		return false;
	}

	if (file_handle->opened_path) {
		return !tc_paths_select(file_handle->opened_path);
	}

	if (!file_handle->filename) {
		return false;
	}

	resolved = zend_resolve_path(file_handle->filename);

	if (!resolved) {
		return false;
	}

	selected = tc_paths_select(resolved);
	zend_string_release(resolved);

	return !selected;
}

static zend_op_array *tc_compile_file(zend_file_handle *file_handle, int type)
{
	uint32_t first_function = CG(function_table)->nNumUsed;
	uint32_t first_class = CG(class_table)->nNumUsed;
	uint32_t compiler_options = CG(compiler_options);
	zend_op_array *op_array;

	if (tc_can_keep_jump_tables(file_handle)) {
		CG(compiler_options) &= ~ZEND_COMPILE_NO_JUMPTABLES;
		tc_stat_jump_table_files++;
	}

	op_array = tc_orig_compile_file(file_handle, type);
	CG(compiler_options) = compiler_options;

	tc_after_compile(op_array, first_function, first_class);

	return op_array;
}

static zend_op_array *tc_compile_string(zend_string *source_string, const char *filename, zend_compile_position position)
{
	uint32_t first_function = CG(function_table)->nNumUsed;
	uint32_t first_class = CG(class_table)->nNumUsed;
	zend_op_array *op_array = tc_orig_compile_string(source_string, filename, position);

	tc_after_compile(op_array, first_function, first_class);

	return op_array;
}

/* The engine calls this function after it freed the opcodes. Only the address is still valid. */
void tc_op_array_dtor(zend_op_array *op_array)
{
	tc_unit *unit = op_array->reserved[tc_slot];

	if ((uintptr_t) unit <= (uintptr_t) TC_NO_PROBES) {
		return;
	}

	/* ext-parallel copies op arrays to its threads together with the slot. The unit belongs to the owner thread. */
	if (!tc_thread_is_owner()) {
		return;
	}

	op_array->reserved[tc_slot] = NULL;

	if (tc_active) {
		zend_hash_index_del(&tc_units, (zend_ulong) (uintptr_t) unit->opcodes);
	}

	unit->opcodes = NULL;

	/* A unit with recorded hits stays until the next clear, because collect needs its lines. */
	if (unit->pending == 0) {
		tc_unit_free(unit);
	}
}

/*
 * This function runs at the first request and not in MINIT.
 * Thus the hooks wrap the hooks of the extensions that started before, and code that OPcache preloads keeps its handlers.
 */
void tc_install(void)
{
	zend_op probe_op;

	tc_register_handlers();

	/* While the handlers are registered, every opcode gets the ZEND_USER_OPCODE handler. This is the value that a probe needs. */
	memset(&probe_op, 0, sizeof(zend_op));
	probe_op.opcode = ZEND_NOP;
	zend_vm_set_opcode_handler(&probe_op);
	tc_probe_label = probe_op.handler;

	tc_orig_compile_file = zend_compile_file;
	zend_compile_file = tc_compile_file;
	tc_orig_compile_string = zend_compile_string;
	zend_compile_string = tc_compile_string;
	tc_installed = true;
}

void tc_compiler_activate(void)
{
	zend_hash_init(&tc_files, 256, NULL, tc_files_dtor, 1);
	zend_hash_init(&tc_units, 256, NULL, NULL, 1);

	/*
	 * A switch with a jump table does not run the comparison of each case. PCOV turns jump tables off too.
	 * tc_compile_file() turns them on again for a file that is not selected.
	 */
	CG(compiler_options) |= ZEND_COMPILE_NO_JUMPTABLES;
}

void tc_compiler_deactivate(void)
{
	zend_hash_destroy(&tc_units);
	zend_hash_destroy(&tc_files);
}
