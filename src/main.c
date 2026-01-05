// src/main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "repl.h"

 int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    repl_run();
    return 0;
}