/* The functions of PCOV. Only the build with --enable-turbocov-impersonate-pcov has this file. */

#include "turbocov_internal.h"

#include "arginfo.h"

static void tc_add_file(zval *result, tc_file *file)
{
	zval lines;

	ZVAL_ARR(&lines, zend_array_dup(&file->lines));
	zend_hash_update(Z_ARRVAL_P(result), file->name, &lines);
}

/* {{{ void start() */
PHP_NAMED_FUNCTION(tc_function_start)
{
	ZEND_PARSE_PARAMETERS_NONE();

	if (tc_threads_refuse()) {
		return;
	}

	/* The PCOV driver of PHPUnit does not expect an exception from start(). */
	if (*tc_off_reason && !tc_off_warned) {
		tc_off_warned = true;
		zend_error(E_WARNING, TURBOCOV_NAME ": the coverage collection is off, because %s", tc_off_reason);
	}

	if (!tc_active) {
		return;
	}

	tc_refs_rearm(&tc_strays);
	tc_collecting = true;
}
/* }}} */

/* {{{ array collect(int $type = all, array $filter = []) */
PHP_NAMED_FUNCTION(tc_function_collect)
{
	zend_long type = TC_FILTER_ALL;
	HashTable *filter = NULL;
	zval *entry;
	zval *known;

	ZEND_PARSE_PARAMETERS_START(0, 2)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(type)
		Z_PARAM_ARRAY_HT(filter)
	ZEND_PARSE_PARAMETERS_END();

	if (type != TC_FILTER_ALL && type != TC_FILTER_INCLUDE && type != TC_FILTER_EXCLUDE) {
		zend_type_error("type must be \\" TURBOCOV_NAME "\\inclusive, \\" TURBOCOV_NAME "\\exclusive, or \\" TURBOCOV_NAME "\\all");
		RETURN_THROWS();
	}

	array_init(return_value);

	if (tc_threads_refuse() || !tc_active || !tc_dirty) {
		return;
	}

	tc_dirty = false;

	if (type == TC_FILTER_INCLUDE) {
		if (filter) {
			ZEND_HASH_FOREACH_VAL(filter, entry) {
				if (Z_TYPE_P(entry) == IS_STRING && (known = zend_hash_find(&tc_files, Z_STR_P(entry))) && Z_PTR_P(known)) {
					tc_add_file(return_value, Z_PTR_P(known));
				}
			} ZEND_HASH_FOREACH_END();
		}
	} else {
		ZEND_HASH_MAP_FOREACH_VAL(&tc_files, known) {
			tc_file *file = Z_PTR_P(known);
			bool skip = false;

			if (!file) {
				continue;
			}

			if (type == TC_FILTER_EXCLUDE && filter) {
				ZEND_HASH_FOREACH_VAL(filter, entry) {
					if (Z_TYPE_P(entry) == IS_STRING && zend_string_equals(file->name, Z_STR_P(entry))) {
						skip = true;
						break;
					}
				} ZEND_HASH_FOREACH_END();
			}

			if (!skip) {
				tc_add_file(return_value, file);
			}
		} ZEND_HASH_FOREACH_END();
	}

	for (size_t i = 0; i < tc_hits.count; i++) {
		tc_unit *unit = tc_hits.refs[i].unit;
		zval *lines = zend_hash_find(Z_ARRVAL_P(return_value), unit->file->name);
		zval *line;

		if (lines && (line = zend_hash_index_find(Z_ARRVAL_P(lines), unit->probes[tc_hits.refs[i].probe].line))) {
			ZVAL_LONG(line, TC_COVERED);
		}
	}
}
/* }}} */

/* {{{ void stop() */
PHP_NAMED_FUNCTION(tc_function_stop)
{
	ZEND_PARSE_PARAMETERS_NONE();

	if (tc_threads_refuse()) {
		return;
	}

	tc_collecting = false;
}
/* }}} */

/* {{{ void clear(bool $files = false) */
PHP_NAMED_FUNCTION(tc_function_clear)
{
	bool files = false;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_BOOL(files)
	ZEND_PARSE_PARAMETERS_END();

	if (tc_threads_refuse() || !tc_active) {
		return;
	}

	tc_refs_rearm(&tc_hits);
	tc_waiting_reset();
	tc_dirty = false;
}
/* }}} */

/* {{{ array waiting() */
PHP_NAMED_FUNCTION(tc_function_waiting)
{
	ZEND_PARSE_PARAMETERS_NONE();

	array_init(return_value);

	if (tc_threads_refuse()) {
		return;
	}

	for (size_t i = 0; i < tc_waiting_count; i++) {
		add_next_index_str(return_value, zend_string_copy(tc_waiting[i]->name));
	}
}
/* }}} */

/* {{{ int memory() */
PHP_NAMED_FUNCTION(tc_function_memory)
{
	ZEND_PARSE_PARAMETERS_NONE();

	if (tc_threads_refuse()) {
		RETURN_LONG(0);
	}

	RETURN_LONG((zend_long) ((tc_hits.capacity + tc_strays.capacity) * sizeof(tc_ref)));
}
/* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(tc_arginfo_collect, 0, 0, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, type, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, filter, IS_ARRAY, 0, "[]")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(tc_arginfo_clear, 0, 0, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, files, _IS_BOOL, 0, "false")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(tc_arginfo_int, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

const zend_function_entry tc_functions[] = {
	ZEND_NS_FENTRY(TURBOCOV_NAME, start, tc_function_start, tc_arginfo_void, 0)
	ZEND_NS_FENTRY(TURBOCOV_NAME, stop, tc_function_stop, tc_arginfo_void, 0)
	ZEND_NS_FENTRY(TURBOCOV_NAME, collect, tc_function_collect, tc_arginfo_collect, 0)
	ZEND_NS_FENTRY(TURBOCOV_NAME, clear, tc_function_clear, tc_arginfo_clear, 0)
	ZEND_NS_FENTRY(TURBOCOV_NAME, waiting, tc_function_waiting, tc_arginfo_array, 0)
	ZEND_NS_FENTRY(TURBOCOV_NAME, memory, tc_function_memory, tc_arginfo_int, 0)
	ZEND_NS_FENTRY(TURBOCOV_NAME, get_status, tc_function_get_status, tc_arginfo_array, 0)
	ZEND_NS_FENTRY(TURBOCOV_NAME, get_debug_info, tc_function_get_debug_info, tc_arginfo_get_debug_info, 0)
	PHP_FE_END
};
