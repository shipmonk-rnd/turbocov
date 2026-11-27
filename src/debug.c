#include "turbocov_internal.h"

static void tc_debug_add_unit(zval *units, tc_unit *unit, zend_op_array *op_array)
{
	static const char *states[] = {"armed", "fired", "fired while the collection was off"};
	zval entry;
	zval probes;

	array_init(&entry);
	array_init(&probes);

	if (!op_array || !op_array->function_name) {
		add_assoc_string(&entry, "name", "{main}");
	} else if (op_array->scope) {
		add_assoc_str(&entry, "name", zend_strpprintf(0, "%s::%s", ZSTR_VAL(op_array->scope->name), ZSTR_VAL(op_array->function_name)));
	} else {
		add_assoc_str(&entry, "name", zend_string_copy(op_array->function_name));
	}

	for (uint32_t i = 0; i < unit->count; i++) {
		tc_probe *probe = &unit->probes[i];
		zval item;

		array_init(&item);
		add_assoc_long(&item, "line", probe->line);
		add_assoc_long(&item, "opnum", probe->opnum);
		add_assoc_string(&item, "opcode", zend_get_opcode_name(unit->opcodes[probe->opnum].opcode));
		add_assoc_string(&item, "state", states[probe->state]);
		add_next_index_zval(&probes, &item);
	}

	add_assoc_zval(&entry, "probes", &probes);
	add_next_index_zval(units, &entry);
}

/* A unit does not keep the name of its function. This function finds the name in the tables of the engine. */
static void tc_debug_visit(zval *units, HashTable *seen, tc_file *file, zend_op_array *op_array)
{
	zend_ulong key;
	tc_unit *unit;

	if (!ZEND_USER_CODE(op_array->type) || !op_array->opcodes) {
		return;
	}

	key = (zend_ulong) (uintptr_t) op_array->opcodes;
	unit = zend_hash_index_find_ptr(&tc_units, key);

	if (unit && unit->file == file && !zend_hash_index_exists(seen, key)) {
		zend_hash_index_add_empty_element(seen, key);
		tc_debug_add_unit(units, unit, op_array);
	}

	for (uint32_t i = 0; i < op_array->num_dynamic_func_defs; i++) {
		tc_debug_visit(units, seen, file, op_array->dynamic_func_defs[i]);
	}
}

static void tc_debug_units(zval *units, tc_file *file)
{
	HashTable seen;
	zend_function *function;
	zend_class_entry *ce;
	zend_ulong key;
	tc_unit *unit;

	zend_hash_init(&seen, 16, NULL, NULL, 0);

	ZEND_HASH_MAP_FOREACH_PTR(EG(function_table), function) {
		tc_debug_visit(units, &seen, file, &function->op_array);
	} ZEND_HASH_FOREACH_END();

	ZEND_HASH_MAP_FOREACH_PTR(EG(class_table), ce) {
		if (ce->type != ZEND_USER_CLASS) {
			continue;
		}

		ZEND_HASH_MAP_FOREACH_PTR(&ce->function_table, function) {
			tc_debug_visit(units, &seen, file, &function->op_array);
		} ZEND_HASH_FOREACH_END();

		if (ce->num_hooked_props) {
			zend_property_info *property;

			ZEND_HASH_MAP_FOREACH_PTR(&ce->properties_info, property) {
				if (property->hooks) {
					for (uint32_t i = 0; i < ZEND_PROPERTY_HOOK_COUNT; i++) {
						if (property->hooks[i]) {
							tc_debug_visit(units, &seen, file, &property->hooks[i]->op_array);
						}
					}
				}
			} ZEND_HASH_FOREACH_END();
		}
	} ZEND_HASH_FOREACH_END();

	/* The code of a file outside of all functions, and the closures in it, are in no table. */
	ZEND_HASH_FOREACH_NUM_KEY_PTR(&tc_units, key, unit) {
		if (unit->file == file && unit->opcodes && !zend_hash_index_exists(&seen, key)) {
			tc_debug_add_unit(units, unit, NULL);
		}
	} ZEND_HASH_FOREACH_END();

	zend_hash_destroy(&seen);
}

static int tc_compare_lines(const void *left, const void *right)
{
	zend_ulong a = *(const zend_ulong *) left;
	zend_ulong b = *(const zend_ulong *) right;

	return a < b ? -1 : (a > b ? 1 : 0);
}

/* {{{ array get_status() */
PHP_NAMED_FUNCTION(tc_function_get_status)
{
	zval *known;
	zend_long files = 0;

	ZEND_PARSE_PARAMETERS_NONE();

	array_init(return_value);

	if (tc_threads_refuse()) {
		return;
	}

	if (tc_active) {
		ZEND_HASH_MAP_FOREACH_VAL(&tc_files, known) {
			if (Z_PTR_P(known)) {
				files++;
			}
		} ZEND_HASH_FOREACH_END();
	}

	if (*tc_off_reason) {
		add_assoc_string(return_value, "disabled_reason", tc_off_reason);
	} else {
		add_assoc_null(return_value, "disabled_reason");
	}

	add_assoc_bool(return_value, "collecting", tc_collecting);
	add_assoc_long(return_value, "collected_files", files);
	add_assoc_long(return_value, "installed_probes", (zend_long) tc_stat_probes);
	add_assoc_long(return_value, "fired_probes", (zend_long) tc_stat_fires);
	add_assoc_long(return_value, "jump_table_files", (zend_long) tc_stat_jump_table_files);
	add_assoc_long(return_value, "live_units", tc_installed ? (zend_long) zend_hash_num_elements(&tc_units) : 0);
}
/* }}} */

/* {{{ array get_debug_info(string $file) */
PHP_NAMED_FUNCTION(tc_function_get_debug_info)
{
	zend_string *filename;
	zend_string *reason;
	zval lines;
	zval units;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(filename)
	ZEND_PARSE_PARAMETERS_END();

	array_init(return_value);

	if (tc_threads_refuse()) {
		return;
	}

	array_init(&lines);
	array_init(&units);

	if (!tc_active) {
		add_assoc_string(return_value, "skip_reason", tc_off_reason);
	} else {
		reason = tc_skip_reason(filename);

		if (reason) {
			add_assoc_str(return_value, "skip_reason", reason);
		} else {
			tc_file *file = Z_PTR_P(zend_hash_find(&tc_files, filename));
			uint32_t count = zend_hash_num_elements(&file->lines);
			zend_ulong *sorted = safe_emalloc(count ? count : 1, sizeof(zend_ulong), 0);
			zend_ulong line;
			uint32_t next = 0;

			add_assoc_null(return_value, "skip_reason");

			ZEND_HASH_FOREACH_NUM_KEY(&file->lines, line) {
				sorted[next++] = line;
			} ZEND_HASH_FOREACH_END();

			qsort(sorted, count, sizeof(zend_ulong), tc_compare_lines);

			for (uint32_t i = 0; i < count; i++) {
				add_next_index_long(&lines, (zend_long) sorted[i]);
			}

			efree(sorted);
			tc_debug_units(&units, file);
		}
	}

	add_assoc_zval(return_value, "executable_lines", &lines);
	add_assoc_zval(return_value, "units", &units);
}
/* }}} */
