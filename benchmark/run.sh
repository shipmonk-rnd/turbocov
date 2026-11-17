#!/bin/sh
# Runs the driver-only benchmark and prints the result. See benchmark/README.md.
# Usage: benchmark/run.sh [rounds]
# Environment:
#   BENCHMARK_SUITES    the names of the suites to run, default: all
#   BENCHMARK_WORK_DIR  the directory for the suites, the logs and the raw times, default: benchmark/work
set -eu
export LC_ALL=C

rounds="${1:-8}"
root="$(cd "$(dirname "$0")/.." && pwd)"
work="${BENCHMARK_WORK_DIR:-$root/benchmark/work}"
image="turbocov-benchmark"
container="turbocov-benchmark-$$"
modes="none turbocov pcov xdebug"
check=""

# name|repository|commit or tag|source directory
suites="
dead-code-detector|https://github.com/shipmonk-rnd/dead-code-detector|ae624a696b5c835650b250c85928ae943c1ecb77|src
carbon|https://github.com/briannesbitt/Carbon|c997755458213c6e74e90637fec095e6cc47093d|src/Carbon
commonmark|https://github.com/thephpleague/commonmark|2.10.7|src
"

mkdir -p "$work/logs"
times="$work/times-$(date +%Y%m%d-%H%M%S).tsv"
commit="$(git -C "$root" describe --always --dirty 2> /dev/null || echo unknown)"

# timeout stops only the docker client. The trap and the name remove a container that still runs.
trap 'docker rm -f "$container" > /dev/null 2>&1 || true' EXIT

# Usage: in_suite <suite> <timeout in seconds> <command>...
in_suite() {
    suite="$1"
    limit="$2"
    shift 2

    timeout "$limit" docker run --rm --init --name "$container" --memory 6g --memory-swap 6g \
        -u "$(id -u):$(id -g)" -e HOME=/tmp -e COMPOSER_HOME=/work/composer -e "TURBOCOV_COMMIT=$commit" \
        -e "TURBOCOV_BENCHMARK_CHECK=$check" \
        -v "$work:/work" -v "$root/benchmark/harness:/harness:ro" -w "/work/$suite" \
        "$image" "$@" || {
            status=$?
            docker rm -f "$container" > /dev/null 2>&1 || true
            return "$status"
        }
}

# Usage: run_suite <suite> <source directory> <mode> <log name>
# Prints the time that PHPUnit reports, in seconds.
run_suite() {
    log="$work/logs/$1-$4.log"

    if ! in_suite "$1" 600 /harness/phpunit.sh "$3" "$2" > "$log" 2>&1; then
        echo "The suite $1 failed in the mode $3. See $log" >&2
        exit 1
    fi

    awk '$1 == "Time:" { n = split($2, part, ":"); s = 0; for (i = 1; i <= n; i++) s = s * 60 + part[i]; printf "%.3f\n", s }' "$log"
}

timeout 1800 docker build -q -f "$root/benchmark/Dockerfile" -t "$image" "$root" > /dev/null

echo "$suites" | while IFS='|' read -r name repository ref source; do
    [ -n "$name" ] || continue

    case " ${BENCHMARK_SUITES:-$name} " in
        *" $name "*) ;;
        *) continue ;;
    esac

    if [ "$(cat "$work/$name/.benchmark-ref" 2> /dev/null)" != "$ref" ]; then
        if [ -e "$work/$name" ]; then
            echo "$work/$name is not at $ref. Delete the directory and start again." >&2
            exit 1
        fi

        mkdir -p "$work/$name"
        in_suite "$name" 1800 /harness/fetch.sh "$repository" "$ref" < /dev/null
        echo "$ref" > "$work/$name/.benchmark-ref"
    fi

    echo "## $name ($repository at $ref)"
    echo
    in_suite "$name" 120 /harness/versions.sh < /dev/null
    echo

    for mode in $modes; do
        check="/work/logs/$name-check-$mode.json"
        run_suite "$name" "$source" "$mode" "check-$mode" < /dev/null > /dev/null
    done
    check=""

    echo "Check run, not timed:"
    echo
    # shellcheck disable=SC2046
    in_suite "$name" 120 php /harness/check.php $(for mode in $modes; do echo "/work/logs/$name-check-$mode.json"; done) < /dev/null
    echo

    round=1
    while [ "$round" -le "$rounds" ]; do
        for mode in $modes; do
            seconds="$(run_suite "$name" "$source" "$mode" "$round-$mode" < /dev/null)"
            printf '%s\t%s\t%s\t%s\n' "$name" "$round" "$mode" "$seconds" >> "$times"
        done
        round=$((round + 1))
    done

    in_suite "$name" 120 php /harness/stats.php "/work/$(basename "$times")" "$name" < /dev/null
    echo
done

echo "Raw times: $times"
