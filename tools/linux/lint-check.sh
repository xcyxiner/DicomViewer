cmake -D FORMAT_COMMAND=clang-format-18 -D FIX=YES -P cmake/lint.cmake
cmake -D FIX=YES -P cmake/spell.cmake