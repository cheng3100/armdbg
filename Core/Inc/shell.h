#ifndef __SHELL_H__
#define __SHELL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "main.h"

typedef struct ShellCommand {
  const char *command;
  int (*handler)(int argc, char *argv[]);
  const char *help;
} sShellCommand;

extern const sShellCommand *const g_shell_commands;
extern const size_t g_num_shell_commands;

typedef struct ShellImpl {
  //! Function to call whenever a character needs to be sent out.
  int (*send_char)(char c);
} sShellImpl;

//! Initializes the demo shell. To be called early at boot.
void shell_boot(const sShellImpl *impl);

//! Call this when a character is received. The character is processed synchronously.
void shell_receive_char(char c);

//! Print help command
int shell_help_handler(int argc, char *argv[]);

//! Prints a line then a newline
void shell_put_line(const char *str);

bool shell_port_getchar(char *c_out);

__attribute__((noreturn))
void shell_processing_loop(void);

void logp(const char *fmt, ...);

#endif

