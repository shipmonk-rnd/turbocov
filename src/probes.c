#include "turbocov_internal.h"

bool tc_collecting;
bool tc_dirty;
const void *tc_probe_label;
tc_refs tc_hits;
tc_refs tc_strays;
tc_file **tc_waiting;
size_t tc_waiting_count;
zend_ulong tc_stat_fires;

static size_t tc_waiting_capacity;

/* Handlers that other extensions registered before turbocov. The probe callback forwards to them. */
static user_opcode_handler_t tc_previous_handlers[256];

static void tc_refs_push(tc_refs *list, tc_unit *unit, uint32_t probe)
{
	if (list->count == list->capacity) {
		list->capacity = list->capacity ? list->capacity * 2 : 1024;
		list->refs = perealloc(list->refs, list->capacity * sizeof(tc_ref), 1);
	}

	list->refs[list->count].unit = unit;
	list->refs[list->count].probe = probe;
	list->count++;
	unit->pending++;
}

void tc_unit_free(tc_unit *unit)
{
	pefree(unit->index, 1);
	pefree(unit->probes, 1);
	pefree(unit, 1);
}

/* Arms the listed probes again and empties the list. */
void tc_refs_rearm(tc_refs *list)
{
	for (size_t i = 0; i < list->count; i++) {
		tc_unit *unit = list->refs[i].unit;
		tc_probe *probe = &unit->probes[list->refs[i].probe];

		probe->state = TC_ARMED;
		unit->pending--;

		if (unit->opcodes) {
			unit->opcodes[probe->opnum].handler = tc_probe_label;
		} else if (unit->pending == 0) {
			tc_unit_free(unit);
		}
	}

	list->count = 0;
}

void tc_waiting_reset(void)
{
	for (size_t i = 0; i < tc_waiting_count; i++) {
		tc_waiting[i]->waiting = false;
	}

	tc_waiting_count = 0;
}

/* Records one fired probe. The caller puts the original handler back. */
static zend_always_inline void tc_record(tc_unit *unit, uint32_t index)
{
	tc_probe *probe = &unit->probes[index];

	tc_stat_fires++;

	if (tc_collecting) {
		tc_file *file = unit->file;

		probe->state = TC_HIT;
		tc_refs_push(&tc_hits, unit, index);
		tc_dirty = true;

		if (!file->waiting) {
			file->waiting = true;

			if (tc_waiting_count == tc_waiting_capacity) {
				tc_waiting_capacity = tc_waiting_capacity ? tc_waiting_capacity * 2 : 64;
				tc_waiting = perealloc(tc_waiting, tc_waiting_capacity * sizeof(tc_file *), 1);
			}

			tc_waiting[tc_waiting_count++] = file;
		}
	} else {
		probe->state = TC_STRAY;
		tc_refs_push(&tc_strays, unit, index);
	}
}

static zend_always_inline int tc_forward(zend_execute_data *execute_data)
{
	user_opcode_handler_t previous = tc_previous_handlers[EX(opline)->opcode];

	return previous ? previous(execute_data) : ZEND_USER_OPCODE_DISPATCH;
}

/*
 * The VM calls this function for an opcode that has the ZEND_USER_OPCODE handler.
 * A probe records its line and puts the original handler back.
 * ZEND_USER_OPCODE_CONTINUE makes the VM dispatch the same opcode again, now with the original handler.
 */
static int tc_probe_handler(zend_execute_data *execute_data)
{
	const zend_op *opline = EX(opline);
	zend_function *func = EX(func);
	tc_unit *unit;
	tc_probe *probe;
	uint32_t number;

	if (UNEXPECTED(!func || !ZEND_USER_CODE(func->type))) {
		return tc_forward(execute_data);
	}

	if (UNEXPECTED(tc_second_thread) && !tc_thread_is_owner()) {
		return tc_forward(execute_data);
	}

	unit = func->op_array.reserved[tc_slot];

	/* EG(exception_op) and op arrays that no compile hook saw are not in a unit. */
	if (UNEXPECTED((uintptr_t) unit <= (uintptr_t) TC_NO_PROBES)
		|| UNEXPECTED(opline < unit->opcodes)
		|| UNEXPECTED(opline >= unit->opcodes + unit->last)) {
		return tc_forward(execute_data);
	}

	number = unit->index[opline - unit->opcodes];

	if (UNEXPECTED(number == 0)) {
		return tc_forward(execute_data);
	}

	probe = &unit->probes[number - 1];

	/*
	 * An extension that replaces zend_execute_ex (PCOV, phpdbg) calls handlers through zend_vm_call_opcode_handler().
	 * That function ignores opline->handler, thus a probe that already fired comes here again.
	 * The same occurs for an opcode that has a handler of a different extension, because its original handler is ZEND_USER_OPCODE too.
	 */
	if (UNEXPECTED(probe->state != TC_ARMED)) {
		return tc_forward(execute_data);
	}

	((zend_op *) opline)->handler = probe->handler;
	tc_record(unit, number - 1);

	return ZEND_USER_OPCODE_CONTINUE;
}

/*
 * ZEND_INCLUDE_OR_EVAL does not execute a file that has only the final "return" opcode, which is the case for a file with declarations only.
 * It does this only with the stock zend_execute_ex. PCOV replaces zend_execute_ex, thus PCOV sees that opcode run and reports the file.
 */
void tc_record_skipped_return(zend_op_array *op_array)
{
	zend_execute_data *caller = EG(current_execute_data);
	tc_unit *unit = op_array->reserved[tc_slot];
	uint32_t number;

	if ((uintptr_t) unit <= (uintptr_t) TC_NO_PROBES
		|| op_array->last != 1
		|| op_array->opcodes[0].opcode != ZEND_RETURN
		|| op_array->opcodes[0].op1_type != IS_CONST
		|| zend_execute_ex != execute_ex) {
		return;
	}

	if (!caller || !caller->func || !ZEND_USER_CODE(caller->func->type) || !caller->opline || caller->opline->opcode != ZEND_INCLUDE_OR_EVAL) {
		return;
	}

	number = unit->index[0];

	if (number == 0 || unit->probes[number - 1].state != TC_ARMED) {
		return;
	}

	op_array->opcodes[0].handler = unit->probes[number - 1].handler;
	tc_record(unit, number - 1);
}

/*
 * The ZEND_USER_OPCODE handler finds the callback through the real opcode of the current instruction.
 * A probe can be on every opcode, thus every opcode needs the callback.
 */
void tc_register_handlers(void)
{
	for (int opcode = 0; opcode < 256; opcode++) {
		if (opcode != ZEND_USER_OPCODE) {
			tc_previous_handlers[opcode] = zend_get_user_opcode_handler((uint8_t) opcode);
			zend_set_user_opcode_handler((uint8_t) opcode, tc_probe_handler);
		}
	}
}

/* Opcodes that have a handler of a different extension keep the ZEND_USER_OPCODE handler, as without turbocov. */
void tc_unregister_handlers(void)
{
	for (int opcode = 0; opcode < 256; opcode++) {
		if (opcode != ZEND_USER_OPCODE) {
			zend_set_user_opcode_handler((uint8_t) opcode, tc_previous_handlers[opcode]);
		}
	}
}
