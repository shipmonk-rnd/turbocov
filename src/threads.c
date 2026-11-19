/* The guard for thread-safe PHP: the state of turbocov belongs to one PHP thread. */

#include "turbocov_internal.h"

#ifdef ZTS

bool tc_second_thread;

static MUTEX_T tc_threads_mutex;
static THREAD_T tc_owner;
static bool tc_has_owner;

void tc_threads_startup(void)
{
	tc_threads_mutex = tsrm_mutex_alloc();
}

void tc_threads_shutdown(void)
{
	if (tc_threads_mutex) {
		tsrm_mutex_free(tc_threads_mutex);
		tc_threads_mutex = NULL;
	}
}

void tc_threads_lock(void)
{
	tsrm_mutex_lock(tc_threads_mutex);
}

void tc_threads_unlock(void)
{
	tsrm_mutex_unlock(tc_threads_mutex);
}

bool tc_thread_is_owner(void)
{
	return tc_has_owner && tc_owner == tsrm_thread_id();
}

/* The caller holds the lock. The thread of the first request becomes the owner. */
bool tc_threads_claim(void)
{
	if (!tc_has_owner) {
		tc_owner = tsrm_thread_id();
		tc_has_owner = true;

		return true;
	}

	if (tc_owner == tsrm_thread_id()) {
		return true;
	}

	tc_second_thread = true;

	return false;
}

bool tc_threads_refuse(void)
{
	if (EXPECTED(!tc_second_thread)) {
		return false;
	}

	if (tc_thread_is_owner()) {
		if (tc_active && !*tc_off_reason) {
			snprintf(tc_off_reason, sizeof(tc_off_reason), TC_SECOND_THREAD_REASON);
			tc_collecting = false;
		}

		return false;
	}

#ifdef TURBOCOV_IMPERSONATE_PCOV
	zend_error(E_WARNING, TURBOCOV_NAME ": the coverage collection is off, because " TC_SECOND_THREAD_REASON);
#else
	zend_throw_exception(tc_exception_ce, "turbocov cannot collect coverage: " TC_SECOND_THREAD_REASON, 0);
#endif

	return true;
}

#endif
