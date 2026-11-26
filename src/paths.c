#include "turbocov_internal.h"

typedef struct _tc_paths {
	HashTable files;       /* absolute path => nothing */
	HashTable directories; /* absolute path with a slash at the end => nothing */
} tc_paths;

static tc_paths tc_collect_paths;
static tc_paths tc_exclude_paths;
static pcre_cache_entry *tc_exclude_pattern;

/* Returns the entry of the list that has the file: the file itself or one of its parent directories. */
static zend_string *tc_paths_find(tc_paths *paths, zend_string *filename)
{
	zend_string *directory;

	if (zend_hash_exists(&paths->files, filename)) {
		return filename;
	}

	ZEND_HASH_MAP_FOREACH_STR_KEY(&paths->directories, directory) {
		if (ZSTR_LEN(filename) > ZSTR_LEN(directory)
			&& memcmp(ZSTR_VAL(filename), ZSTR_VAL(directory), ZSTR_LEN(directory)) == 0) {
			return directory;
		}
	} ZEND_HASH_FOREACH_END();

	return NULL;
}

static bool tc_matches_exclude_pattern(zend_string *filename)
{
	zval match;
	bool excluded;

	ZVAL_UNDEF(&match);

#if PHP_VERSION_ID < 80400
	/* PHP 8.3 takes an extra use_flags argument. */
	php_pcre_match_impl(tc_exclude_pattern, filename, &match, NULL, 0, 0, 0, 0);
#else
	php_pcre_match_impl(tc_exclude_pattern, filename, &match, NULL, false, 0, 0);
#endif

	excluded = zend_is_true(&match);
	zval_ptr_dtor(&match);

	return excluded;
}

static void tc_paths_init(tc_paths *paths)
{
	zend_hash_init(&paths->files, 16, NULL, NULL, 1);
	zend_hash_init(&paths->directories, 4, NULL, NULL, 1);
}

static void tc_paths_destroy(tc_paths *paths)
{
	zend_hash_destroy(&paths->files);
	zend_hash_destroy(&paths->directories);
}

/* A path that does not exist is an error: it is a typing mistake or an old list, and the user must know. */
static bool tc_paths_add(tc_paths *paths, const char *setting, const char *entry, size_t length)
{
	char path[MAXPATHLEN];
	char resolved[MAXPATHLEN];
	zend_stat_t status;
	size_t resolved_length;

	while (length > 0 && isspace((unsigned char) *entry)) {
		entry++;
		length--;
	}

	while (length > 0 && isspace((unsigned char) entry[length - 1])) {
		length--;
	}

	if (length == 0) {
		return true;
	}

	if (length >= MAXPATHLEN) {
		snprintf(tc_off_reason, sizeof(tc_off_reason), "%s has a path that is too long", setting);

		return false;
	}

	memcpy(path, entry, length);
	path[length] = '\0';

	if (!VCWD_REALPATH(path, resolved) || VCWD_STAT(resolved, &status) != 0) {
		snprintf(tc_off_reason, sizeof(tc_off_reason), "%s has a path that does not exist: %s", setting, path);

		return false;
	}

	resolved_length = strlen(resolved);

	if (!S_ISDIR(status.st_mode)) {
		zend_hash_str_add_empty_element(&paths->files, resolved, resolved_length);

		return true;
	}

	if (resolved[resolved_length - 1] != DEFAULT_SLASH) {
		if (resolved_length + 1 >= MAXPATHLEN) {
			snprintf(tc_off_reason, sizeof(tc_off_reason), "%s has a path that is too long", setting);

			return false;
		}

		resolved[resolved_length++] = DEFAULT_SLASH;
		resolved[resolved_length] = '\0';
	}

	zend_hash_str_add_empty_element(&paths->directories, resolved, resolved_length);

	return true;
}

/*
 * The value is a list of paths, or "@" and the name of a file that has one path on each line.
 * On an error this function writes the reason to tc_off_reason and returns false.
 */
static bool tc_paths_load(tc_paths *paths, const char *setting, const char *value)
{
	bool valid = true;

	if (!value || !*value) {
		return true;
	}

	if (*value == '@') {
		char line[MAXPATHLEN + 2];
		FILE *list = VCWD_FOPEN(value + 1, "r");

		if (!list) {
			snprintf(tc_off_reason, sizeof(tc_off_reason), "the list file %s of %s is not readable", value + 1, setting);

			return false;
		}

		while (valid && fgets(line, sizeof(line), list)) {
			valid = tc_paths_add(paths, setting, line, strlen(line));
		}

		fclose(list);

		return valid;
	}

	while (valid && *value) {
		const char *end = strchr(value, ZEND_PATHS_SEPARATOR);
		size_t length = end ? (size_t) (end - value) : strlen(value);

		valid = tc_paths_add(paths, setting, value, length);
		value += length + (end ? 1 : 0);
	}

	return valid;
}

static void tc_setup_paths(void)
{
	const char *collect = zend_ini_string_literal(TC_INI_COLLECT_PATHS);

	tc_paths_init(&tc_collect_paths);
	tc_paths_init(&tc_exclude_paths);

	if (!collect || !*collect) {
		snprintf(tc_off_reason, sizeof(tc_off_reason), TC_INI_COLLECT_PATHS " is not set");

		return;
	}

	if (!tc_paths_load(&tc_collect_paths, TC_INI_COLLECT_PATHS, collect)) {
		return;
	}

#ifdef TC_INI_EXCLUDE_PATHS
	tc_paths_load(&tc_exclude_paths, TC_INI_EXCLUDE_PATHS, zend_ini_string_literal(TC_INI_EXCLUDE_PATHS));
#endif
}

static void tc_setup_exclude_pattern(void)
{
	const char *exclude = zend_ini_string_literal(TC_INI_EXCLUDE_PATTERN);
	zend_string *pattern;
	int error_reporting = EG(error_reporting);

	tc_exclude_pattern = NULL;

	if (!exclude || !*exclude) {
		return;
	}

	/* PCRE gives a warning for a pattern that is not valid. The reason goes to the caller of start() and not to the output. */
	EG(error_reporting) = 0;
	pattern = zend_string_init(exclude, strlen(exclude), 0);
	tc_exclude_pattern = pcre_get_compiled_regex_cache(pattern);
	EG(error_reporting) = error_reporting;

	if (tc_exclude_pattern) {
		php_pcre_pce_incref(tc_exclude_pattern);
	} else if (!*tc_off_reason) {
		snprintf(tc_off_reason, sizeof(tc_off_reason), TC_INI_EXCLUDE_PATTERN " is not a valid pattern: %s", exclude);
	}

	zend_string_release(pattern);
}

/* Returns the reason why the file has no data, or NULL. The caller releases the string. */
zend_string *tc_skip_reason(zend_string *filename)
{
	if (*tc_off_reason) {
		return zend_string_init(tc_off_reason, strlen(tc_off_reason), 0);
	}

	if (!tc_paths_find(&tc_collect_paths, filename)) {
		return zend_string_init(ZEND_STRL(TC_INI_COLLECT_PATHS " does not have this file"), 0);
	}

#ifdef TC_INI_EXCLUDE_PATHS
	{
		zend_string *entry = tc_paths_find(&tc_exclude_paths, filename);

		if (entry) {
			return zend_strpprintf(0, TC_INI_EXCLUDE_PATHS " has %s", ZSTR_VAL(entry));
		}
	}
#endif

	if (tc_exclude_pattern && tc_matches_exclude_pattern(filename)) {
		return zend_string_init(ZEND_STRL(TC_INI_EXCLUDE_PATTERN " matches this file"), 0);
	}

	if (!zend_hash_exists(&tc_files, filename)) {
		return zend_string_init(ZEND_STRL("PHP did not compile this file"), 0);
	}

	return NULL;
}

void tc_paths_activate(void)
{
	tc_setup_paths();
	tc_setup_exclude_pattern();
}

void tc_paths_deactivate(void)
{
	tc_paths_destroy(&tc_collect_paths);
	tc_paths_destroy(&tc_exclude_paths);

	if (tc_exclude_pattern) {
		php_pcre_pce_decref(tc_exclude_pattern);
		tc_exclude_pattern = NULL;
	}
}

bool tc_paths_select(zend_string *filename)
{
	return tc_paths_find(&tc_collect_paths, filename)
		&& !tc_paths_find(&tc_exclude_paths, filename)
		&& !(tc_exclude_pattern && tc_matches_exclude_pattern(filename));
}
