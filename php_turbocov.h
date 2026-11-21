#ifndef PHP_TURBOCOV_H
#define PHP_TURBOCOV_H

#define PHP_TURBOCOV_VERSION "0.1.1"

/* PHPUnit selects its PCOV driver when an extension with the name "pcov" is loaded. */
#ifdef TURBOCOV_IMPERSONATE_PCOV
# define TURBOCOV_NAME "pcov"
#else
# define TURBOCOV_NAME "turbocov"
#endif

extern zend_module_entry turbocov_module_entry;
#define phpext_turbocov_ptr &turbocov_module_entry

#endif
