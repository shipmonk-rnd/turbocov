#include "turbocov_internal.h"

bool tc_ini_enabled;
char tc_off_reason[MAXPATHLEN + 128];
bool tc_active;
#ifdef TURBOCOV_IMPERSONATE_PCOV
bool tc_off_warned;
#else
zend_class_entry *tc_exception_ce;
#endif

static zend_extension tc_zend_extension;

PHP_INI_BEGIN()
	PHP_INI_ENTRY(TURBOCOV_NAME ".enabled", "1", PHP_INI_SYSTEM, NULL)
	PHP_INI_ENTRY(TC_INI_COLLECT_PATHS, "", PHP_INI_SYSTEM | PHP_INI_PERDIR, NULL)
#ifdef TC_INI_EXCLUDE_PATHS
	PHP_INI_ENTRY(TC_INI_EXCLUDE_PATHS, "", PHP_INI_SYSTEM | PHP_INI_PERDIR, NULL)
#endif
	PHP_INI_ENTRY(TC_INI_EXCLUDE_PATTERN, "", PHP_INI_SYSTEM | PHP_INI_PERDIR, NULL)
PHP_INI_END()

static PHP_MINIT_FUNCTION(turbocov)
{
#ifdef TURBOCOV_IMPERSONATE_PCOV
	REGISTER_NS_LONG_CONSTANT(TURBOCOV_NAME, "all", TC_FILTER_ALL, CONST_CS | CONST_PERSISTENT);
	REGISTER_NS_LONG_CONSTANT(TURBOCOV_NAME, "inclusive", TC_FILTER_INCLUDE, CONST_CS | CONST_PERSISTENT);
	REGISTER_NS_LONG_CONSTANT(TURBOCOV_NAME, "exclusive", TC_FILTER_EXCLUDE, CONST_CS | CONST_PERSISTENT);
	REGISTER_NS_STRING_CONSTANT(TURBOCOV_NAME, "version", PHP_TURBOCOV_VERSION, CONST_CS | CONST_PERSISTENT);
#else
	zend_class_entry exception_ce;

	INIT_NS_CLASS_ENTRY(exception_ce, TURBOCOV_NAME, "CannotCollectException", NULL);
	tc_exception_ce = zend_register_internal_class_ex(&exception_ce, spl_ce_RuntimeException);
#endif

	REGISTER_INI_ENTRIES();
	tc_threads_startup();

	tc_ini_enabled = INI_BOOL(TURBOCOV_NAME ".enabled");

	if (!tc_ini_enabled) {
		snprintf(tc_off_reason, sizeof(tc_off_reason), TURBOCOV_NAME ".enabled is 0");
#ifdef TURBOCOV_IMPERSONATE_PCOV
		/* The user turned the extension off. start() gives no warning for that. */
		tc_off_warned = true;
#endif

		return SUCCESS;
	}

	tc_slot = zend_get_resource_handle(TURBOCOV_NAME);

	if (tc_slot < 0) {
		zend_error(E_CORE_WARNING, TURBOCOV_NAME ": no free op array slot, the extension is off");
		snprintf(tc_off_reason, sizeof(tc_off_reason), "PHP has no free op array slot");
#ifdef TURBOCOV_IMPERSONATE_PCOV
		tc_off_warned = true;
#endif
		tc_ini_enabled = false;

		return SUCCESS;
	}

	tc_fill_ignored_opcodes();

	memset(&tc_zend_extension, 0, sizeof(zend_extension));
	tc_zend_extension.name = (char *) TURBOCOV_NAME " op array tracker";
	tc_zend_extension.version = (char *) PHP_TURBOCOV_VERSION;
	tc_zend_extension.author = (char *) "";
	tc_zend_extension.URL = (char *) "";
	tc_zend_extension.copyright = (char *) "";
	tc_zend_extension.op_array_dtor = tc_op_array_dtor;
	tc_zend_extension.resource_number = -1;
	zend_register_extension(&tc_zend_extension, NULL);

	return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(turbocov)
{
	if (tc_installed) {
		tc_unregister_handlers();
	}

	UNREGISTER_INI_ENTRIES();
	tc_threads_shutdown();

	return SUCCESS;
}

static PHP_RINIT_FUNCTION(turbocov)
{
	if (!tc_ini_enabled) {
		return SUCCESS;
	}

	/* The lock makes a thread that starts now wait until the handlers of the owner are complete. */
	tc_threads_lock();

	if (!tc_threads_claim()) {
		tc_threads_unlock();

		return SUCCESS;
	}

	if (!tc_installed) {
		const char *conflict = tc_find_conflict();

		if (conflict) {
			/* No message here: most processes do not collect coverage, and a message at startup can break their output. */
			snprintf(tc_off_reason, sizeof(tc_off_reason), "%s", conflict);
			tc_ini_enabled = false;
			tc_threads_unlock();

			return SUCCESS;
		}

		tc_install();
	}

	tc_threads_unlock();

	tc_compiler_activate();

	*tc_off_reason = '\0';
#ifdef TURBOCOV_IMPERSONATE_PCOV
	tc_off_warned = false;
#endif
	tc_paths_activate();

	tc_collecting = false;
	tc_dirty = false;
	tc_active = true;

	return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(turbocov)
{
	if (!tc_active || !tc_thread_is_owner()) {
		return SUCCESS;
	}

	/* The engine destroys the op arrays after this point. The op array destructor then frees each unit. */
	tc_active = false;
	tc_collecting = false;

	tc_refs_rearm(&tc_hits);
	tc_refs_rearm(&tc_strays);
	tc_waiting_reset();

	tc_compiler_deactivate();
	tc_paths_deactivate();

	return SUCCESS;
}

static PHP_MINFO_FUNCTION(turbocov)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "turbocov support", tc_ini_enabled ? "enabled" : "disabled");
	php_info_print_table_row(2, "turbocov version", PHP_TURBOCOV_VERSION);
	php_info_print_table_row(2, "Registered as", TURBOCOV_NAME);
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}

static const zend_module_dep tc_module_deps[] = {
	ZEND_MOD_REQUIRED("pcre")
	ZEND_MOD_REQUIRED("spl")
	ZEND_MOD_END
};

zend_module_entry turbocov_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	tc_module_deps,
	TURBOCOV_NAME,
	tc_functions,
	PHP_MINIT(turbocov),
	PHP_MSHUTDOWN(turbocov),
	PHP_RINIT(turbocov),
	PHP_RSHUTDOWN(turbocov),
	PHP_MINFO(turbocov),
	PHP_TURBOCOV_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_TURBOCOV
ZEND_GET_MODULE(turbocov)
#endif
