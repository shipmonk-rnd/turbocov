#!/bin/sh
# Fetches one commit or tag of a repository into the current directory and installs its dependencies.
# Usage: fetch.sh <repository> <commit or tag>
set -eu

git init -q .
git fetch -q --depth 1 "$1" "$2"
git checkout -q FETCH_HEAD
composer install --no-interaction --no-progress --quiet
