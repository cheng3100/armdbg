#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include "main.h"

//! Prints a line then a newline
void shell_put_line(const char *str);

bool shell_getchar(char *c_out);

int shell_putc(char c);

void logp(const char *fmt, ...);
