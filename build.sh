#!/bin/sh

CFLAGS="-Wall -Wextra -pedantic -ggdb -std=c11"

if [ ! -d ./build/ ]; then
    mkdir -p ./build/
fi;

gcc $CFLAGS -o ./build/cdilla ./src/main.c ./src/utils.c ./src/cdilla_lexer.c ./src/cdilla_parser.c
