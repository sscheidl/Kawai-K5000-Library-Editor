#!/usr/bin/env bash
#
# Guard for OPEN_QUESTIONS Q2: the private K5000 reference corpus must never be
# committed. Only freely redistributable or self-generated fixtures under
# testdata/fixtures/ may contain K5000 binary material.
#
# .gitignore already blocks accidental adds, but ignore rules do not apply to
# files that are already tracked. This checks the actual index, so a file added
# with `git add -f`, or committed before the rule existed, is still caught.
#
# Usage: tools/check-no-corpus-binaries.sh
# Exit:  0 clean, 1 violations found.

set -euo pipefail

cd "$(dirname "$0")/.."

readonly EXTENSIONS='ka1|kaa|kc1|kca|kra|kb1|syx|img'
readonly ALLOWED_PREFIX='testdata/fixtures/'

violations="$(
    git ls-files \
        | grep -Ei "\.(${EXTENSIONS})\$" \
        | grep -v "^${ALLOWED_PREFIX}" \
        || true
)"

if [ -n "${violations}" ]; then
    echo "ERROR: tracked K5000 binary files outside ${ALLOWED_PREFIX}" >&2
    echo >&2
    echo "${violations}" | sed 's/^/  /' >&2
    echo >&2
    echo "The reference corpus is private and must never be committed." >&2
    echo "See docs/OPEN_QUESTIONS.md Q2 and testdata/README.md." >&2
    echo >&2
    echo "To remove a file from tracking while keeping it on disk:" >&2
    echo "  git rm --cached <file>" >&2
    exit 1
fi

# Second guard: fixtures are meant to be small. A multi-megabyte "fixture" is
# almost certainly a corpus file that was filed in the wrong place.
readonly MAX_FIXTURE_BYTES=262144
oversized=0

while IFS= read -r f; do
    [ -f "$f" ] || continue
    size="$(wc -c <"$f" | tr -d '[:space:]')"
    if [ "$size" -gt "$MAX_FIXTURE_BYTES" ]; then
        echo "ERROR: fixture exceeds ${MAX_FIXTURE_BYTES} bytes: $f ($size bytes)" >&2
        oversized=1
    fi
done < <(git ls-files "${ALLOWED_PREFIX}" | grep -Ei "\.(${EXTENSIONS})\$" || true)

if [ "$oversized" -ne 0 ]; then
    echo >&2
    echo "Fixtures must be small and purposeful. Large real-world material" >&2
    echo "belongs in the private corpus, not in the repository." >&2
    exit 1
fi

echo "OK: no tracked K5000 binaries outside ${ALLOWED_PREFIX}"
