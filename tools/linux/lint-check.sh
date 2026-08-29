REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT" || exit 1
cmake -D FORMAT_COMMAND=clang-format-18 -D FIX=YES -P cmake/lint.cmake
cmake -D FIX=YES -P cmake/spell.cmake