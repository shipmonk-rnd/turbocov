FROM php:8.5-cli

ARG PCOV_VERSION=1.0.12
ARG XDEBUG_VERSION=3.5.3

# The tests of Carbon need the locales.
RUN apt-get update -qq \
    && apt-get install -y -qq --no-install-recommends git unzip locales > /dev/null \
    && rm -rf /var/lib/apt/lists/* \
    && for locale in ar_AE en_US fr_FR sr_ME yo_NG zh_CN zh_TW; do echo "$locale.UTF-8 UTF-8" >> /etc/locale.gen; done \
    && locale-gen > /dev/null

# The extensions are installed and not enabled. Each run loads one of them with -d.
RUN MAKEFLAGS=-j4 pecl install "pcov-$PCOV_VERSION" "xdebug-$XDEBUG_VERSION" > /dev/null

COPY --from=composer/composer:2.11.1-bin /composer /usr/bin/composer

COPY config.m4 php_turbocov.h /usr/src/turbocov/
COPY src /usr/src/turbocov/src
RUN cd /usr/src/turbocov \
    && phpize > /dev/null \
    && ./configure --enable-turbocov > /dev/null \
    && make -j4 > /dev/null \
    && make install > /dev/null
