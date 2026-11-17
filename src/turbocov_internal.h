#ifndef TURBOCOV_INTERNAL_H
#define TURBOCOV_INTERNAL_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "SAPI.h"
#include "ext/standard/info.h"
#include "ext/pcre/php_pcre.h"
#include "ext/spl/spl_exceptions.h"
#include "zend_exceptions.h"
#include "zend_extensions.h"
#include "zend_vm.h"
#include "zend_vm_opcodes.h"
#include "zend_bitset.h"
#include "zend_arena.h"
#include "Zend/Optimizer/zend_cfg.h"

#include "php_turbocov.h"

#if PHP_VERSION_ID < 80400
# error "turbocov needs PHP 8.4 or later"
#endif

#ifdef ZTS
# error "turbocov does not support thread-safe PHP"
#endif

/* For the symbols that more than one source file uses. They are not part of the interface of the shared library. */
#if defined(__GNUC__) && __GNUC__ >= 4
# define TC_LOCAL __attribute__((visibility("hidden")))
#else
# define TC_LOCAL
#endif

#ifdef TURBOCOV_IMPERSONATE_PCOV
# define TC_FILTER_ALL     0
# define TC_FILTER_INCLUDE 1
# define TC_FILTER_EXCLUDE 2
#endif

#define TC_UNCOVERED -1
#define TC_COVERED    1

#define TC_ARMED 0
#define TC_HIT   1
#define TC_STRAY 2

/* Value of the op array slot for code that has real handlers and no probes. */
#define TC_NO_PROBES ((void *) 1)

/* The INI settings of PCOV take the same values. They are only other names. */
#ifdef TURBOCOV_IMPERSONATE_PCOV
# define TC_INI_COLLECT_PATHS   TURBOCOV_NAME ".directory"
# define TC_INI_EXCLUDE_PATTERN TURBOCOV_NAME ".exclude"
#else
# define TC_INI_COLLECT_PATHS   TURBOCOV_NAME ".collect_paths"
# define TC_INI_EXCLUDE_PATHS   TURBOCOV_NAME ".exclude_paths"
# define TC_INI_EXCLUDE_PATTERN TURBOCOV_NAME ".exclude_pattern"
#endif

typedef struct _tc_file {
	zend_string *name;
	HashTable lines; /* executable line => TC_UNCOVERED */
	bool waiting;
} tc_file;

typedef struct _tc_probe {
	const void *handler; /* the handler that the probe replaced */
	uint32_t opnum;
	uint32_t line;
	uint8_t state;
} tc_probe;

/* One unit for each opcode array. Copies of an op array (closures, trait methods) share the opcodes and the unit. */
typedef struct _tc_unit {
	zend_op *opcodes; /* NULL after the engine destroyed the op array */
	tc_file *file;
	uint32_t last;
	uint32_t *index; /* opcode number => probe number + 1, or 0 */
	tc_probe *probes;
	uint32_t count;
	uint32_t pending; /* number of references from tc_hits and tc_strays */
} tc_unit;

typedef struct _tc_ref {
	tc_unit *unit;
	uint32_t probe;
} tc_ref;

typedef struct _tc_refs {
	tc_ref *refs;
	size_t count;
	size_t capacity;
} tc_refs;

/* turbocov.c: the module and its state */
TC_LOCAL extern bool tc_ini_enabled;
TC_LOCAL extern char tc_off_reason[MAXPATHLEN + 128]; /* why the collection is off, or an empty string */
TC_LOCAL extern bool tc_active;
#ifdef TURBOCOV_IMPERSONATE_PCOV
TC_LOCAL extern bool tc_off_warned;
#else
TC_LOCAL extern zend_class_entry *tc_exception_ce;
#endif

/* api_native.c or api_pcov.c: the functions for PHP code */
TC_LOCAL extern const zend_function_entry tc_functions[];

/* probes.c: the probe handler and the lists of fired probes */
TC_LOCAL extern bool tc_collecting;
TC_LOCAL extern bool tc_dirty;
TC_LOCAL extern const void *tc_probe_label;
TC_LOCAL extern tc_refs tc_hits;   /* probes that fired while the collection was on */
TC_LOCAL extern tc_refs tc_strays; /* probes that fired while the collection was off */
TC_LOCAL extern tc_file **tc_waiting;
TC_LOCAL extern size_t tc_waiting_count;
TC_LOCAL extern zend_ulong tc_stat_fires;

TC_LOCAL void tc_unit_free(tc_unit *unit);
TC_LOCAL void tc_refs_rearm(tc_refs *list);
TC_LOCAL void tc_waiting_reset(void);
TC_LOCAL void tc_record_skipped_return(zend_op_array *op_array);
TC_LOCAL void tc_register_handlers(void);
TC_LOCAL void tc_unregister_handlers(void);

/* compiler.c: the compile hooks and the installation of probes */
TC_LOCAL extern int tc_slot;
TC_LOCAL extern bool tc_installed;
TC_LOCAL extern HashTable tc_files; /* file name => tc_file, or NULL for a file outside the whitelist */
TC_LOCAL extern HashTable tc_units; /* opcodes address => tc_unit */
TC_LOCAL extern zend_ulong tc_stat_probes;
TC_LOCAL extern zend_ulong tc_stat_jump_table_files;

TC_LOCAL void tc_fill_ignored_opcodes(void);
TC_LOCAL void tc_install(void);
TC_LOCAL void tc_compiler_activate(void);
TC_LOCAL void tc_compiler_deactivate(void);
TC_LOCAL void tc_op_array_dtor(zend_op_array *op_array);

/* paths.c: the settings that select the files */
TC_LOCAL void tc_paths_activate(void);
TC_LOCAL void tc_paths_deactivate(void);
TC_LOCAL bool tc_paths_select(zend_string *filename);
TC_LOCAL zend_string *tc_skip_reason(zend_string *filename);

/* conflicts.c: the setups in which probes cannot work */
TC_LOCAL const char *tc_find_conflict(void);

/* debug.c: the functions for diagnosis, in both builds */
PHP_NAMED_FUNCTION(tc_function_get_status);
PHP_NAMED_FUNCTION(tc_function_get_debug_info);

#endif
