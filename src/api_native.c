/* The functions of the build with the name turbocov. */

#include "turbocov_internal.h"

#include "arginfo.h"

static bool tc_ensure_can_collect(void)
{
	if (tc_threads_refuse()) {
		return false;
	}

	if (*tc_off_reason) {
		zend_throw_exception_ex(tc_exception_ce, 0, "turbocov cannot collect coverage: %s", tc_off_reason);

		return false;
	}

	return true;
}

/* {{{ void ensure_can_collect() */
PHP_NAMED_FUNCTION(tc_function_ensure_can_collect)
{
	ZEND_PARSE_PARAMETERS_NONE();

	tc_ensure_can_collect();
}
/* }}} */

/* {{{ void start() */
PHP_NAMED_FUNCTION(tc_function_start)
{
	ZEND_PARSE_PARAMETERS_NONE();

	if (!tc_ensure_can_collect()) {
		RETURN_THROWS();
	}

	if (!tc_active) {
		return;
	}

	tc_refs_rearm(&tc_strays);
	tc_collecting = true;
}
/* }}} */

/* {{{ array stop() */
PHP_NAMED_FUNCTION(tc_function_stop)
{
	zval covered;

	ZEND_PARSE_PARAMETERS_NONE();

	if (tc_threads_refuse()) {
		RETURN_THROWS();
	}

	tc_collecting = false;

	/* The data has no lines of the other threads, and code that PHP compiled after their start has no probes. */
	if (tc_second_thread && tc_active) {
		tc_refs_rearm(&tc_hits);
		tc_waiting_reset();
		tc_dirty = false;
		tc_ensure_can_collect();
		RETURN_THROWS();
	}

	array_init(return_value);

	if (!tc_active) {
		return;
	}

	for (size_t i = 0; i < tc_waiting_count; i++) {
		zval lines;

		array_init(&lines);
		zend_hash_update(Z_ARRVAL_P(return_value), tc_waiting[i]->name, &lines);
	}

	ZVAL_LONG(&covered, TC_COVERED);

	for (size_t i = 0; i < tc_hits.count; i++) {
		tc_unit *unit = tc_hits.refs[i].unit;
		zval *lines = zend_hash_find(Z_ARRVAL_P(return_value), unit->file->name);

		if (lines) {
			zend_hash_index_update(Z_ARRVAL_P(lines), unit->probes[tc_hits.refs[i].probe].line, &covered);
		}
	}

	tc_refs_rearm(&tc_hits);
	tc_waiting_reset();
	tc_dirty = false;
}
/* }}} */

/* {{{ array get_executable_lines(array $files) */
PHP_NAMED_FUNCTION(tc_function_get_executable_lines)
{
	HashTable *files;
	zval *entry;
	zval *known;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ARRAY_HT(files)
	ZEND_PARSE_PARAMETERS_END();

	if (tc_threads_refuse()) {
		RETURN_THROWS();
	}

	array_init(return_value);

	if (!tc_active) {
		return;
	}

	ZEND_HASH_FOREACH_VAL(files, entry) {
		if (Z_TYPE_P(entry) == IS_STRING && (known = zend_hash_find(&tc_files, Z_STR_P(entry))) && Z_PTR_P(known)) {
			tc_file *file = Z_PTR_P(known);
			zval lines;

			ZVAL_ARR(&lines, zend_array_dup(&file->lines));
			zend_hash_update(Z_ARRVAL_P(return_value), file->name, &lines);
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(tc_arginfo_get_executable_lines, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, files, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

const zend_function_entry tc_functions[] = {
	ZEND_NS_FENTRY(TURBOCOV_NAME, ensure_can_collect, tc_function_ensure_can_collect, tc_arginfo_void, 0)
	ZEND_NS_FENTRY(TURBOCOV_NAME, start, tc_function_start, tc_arginfo_void, 0)
	ZEND_NS_FENTRY(TURBOCOV_NAME, stop, tc_function_stop, tc_arginfo_array, 0)
	ZEND_NS_FENTRY(TURBOCOV_NAME, get_executable_lines, tc_function_get_executable_lines, tc_arginfo_get_executable_lines, 0)
	ZEND_NS_FENTRY(TURBOCOV_NAME, get_status, tc_function_get_status, tc_arginfo_array, 0)
	ZEND_NS_FENTRY(TURBOCOV_NAME, get_debug_info, tc_function_get_debug_info, tc_arginfo_get_debug_info, 0)
	PHP_FE_END
};
