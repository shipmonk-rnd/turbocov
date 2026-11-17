#include "turbocov_internal.h"

static const char *tc_find_extension_conflict(void)
{
	/* Xdebug registers its own opcode handlers. The two extensions are not tested together. */
	if (zend_hash_str_exists(&module_registry, ZEND_STRL("xdebug"))) {
		return "the Xdebug extension is loaded. Do not load it together with " TURBOCOV_NAME;
	}

#ifndef TURBOCOV_IMPERSONATE_PCOV
	/* PCOV replaces the instruction loop of PHP. The probes then cost time on each opcode. */
	if (zend_hash_str_exists(&module_registry, ZEND_STRL("pcov"))) {
		return "the PCOV extension is loaded. Do not load it together with turbocov";
	}
#endif

	return NULL;
}

/*
 * A probe changes opcodes in place. With OPcache the opcodes of cached files are in its shared memory.
 * In a CLI process that memory is private and writable, thus probes work there.
 */
static const char *tc_find_opcache_conflict(void)
{
	bool is_cli = strcmp(sapi_module.name, "cli") == 0 || strcmp(sapi_module.name, "phpdbg") == 0;
	const char *value;

	if (!zend_ini_bool_literal("opcache.enable") || (strcmp(sapi_module.name, "cli") == 0 && !zend_ini_bool_literal("opcache.enable_cli"))) {
		return NULL;
	}

	if (!is_cli) {
		return "OPcache shares the compiled code between the processes of this SAPI";
	}

	if (zend_ini_bool_literal("opcache.protect_memory")) {
		return "opcache.protect_memory makes the compiled code read-only";
	}

	value = zend_ini_string_literal("opcache.file_cache");

	if (value && *value) {
		return "opcache.file_cache stores the compiled code for other processes";
	}

	value = zend_ini_string_literal("opcache.jit");

	if (value && *value && zend_ini_long_literal("opcache.jit_buffer_size") > 0
		&& strcmp(value, "disable") != 0 && strcmp(value, "off") != 0 && strcmp(value, "0") != 0) {
		return "the JIT of OPcache is on";
	}

	/* OPcache compiles the preloaded code before the first request, thus before turbocov has its hooks. */
	value = zend_ini_string_literal("opcache.preload");

	if (value && *value) {
		return "opcache.preload is set. Code that OPcache preloads gets no probes";
	}

	/* The optimizer removes code. A line without code has no probe, and the report shows it as not covered. */
	if (zend_ini_long_literal("opcache.optimization_level") != 0) {
		return "the optimizer of OPcache is on. Set opcache.optimization_level=0";
	}

	return NULL;
}

/* Returns the reason when probes cannot work in this process, else NULL. */
const char *tc_find_conflict(void)
{
	const char *conflict = tc_find_extension_conflict();

	return conflict ? conflict : tc_find_opcache_conflict();
}
