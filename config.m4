PHP_ARG_ENABLE([turbocov],
  [whether to enable turbocov],
  [AS_HELP_STRING([--enable-turbocov], [Enable turbocov])])

PHP_ARG_ENABLE([turbocov-impersonate-pcov],
  [whether turbocov registers under the name pcov],
  [AS_HELP_STRING([--enable-turbocov-impersonate-pcov], [Register the extension, its functions and its INI settings under the name pcov])],
  [no], [no])

if test "$PHP_TURBOCOV" != "no"; then
  turbocov_api="src/api_native.c"

  if test "$PHP_TURBOCOV_IMPERSONATE_PCOV" != "no"; then
    AC_DEFINE([TURBOCOV_IMPERSONATE_PCOV], [1], [Define to 1 to register turbocov under the name pcov.])
    turbocov_api="src/api_pcov.c"
  fi

  dnl php_pcre.h of a PHP with an external PCRE2 includes "pcre2.h". The compiler does not search the directory of that file on each system.
  if ! grep -q '^#define HAVE_BUNDLED_PCRE 1' "$phpincludedir/main/php_config.h" 2>/dev/null; then
    PKG_CHECK_MODULES([PCRE2], [libpcre2-8],
      [PHP_EVAL_INCLINE([$PCRE2_CFLAGS])],
      [AC_MSG_WARN([pkg-config does not find libpcre2-8. The compiler must find pcre2.h without help.])])
  fi

  PHP_NEW_EXTENSION([turbocov],
    [src/turbocov.c src/compiler.c src/probes.c src/paths.c src/conflicts.c src/debug.c src/threads.c $turbocov_api],
    [$ext_shared])
  PHP_ADD_BUILD_DIR([$ext_builddir/src])
  PHP_ADD_INCLUDE([$ext_srcdir])
fi
