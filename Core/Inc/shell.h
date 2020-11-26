#ifndef __SHELL_H__
#define __SHELL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "main.h"

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

void prv_echo_str(const char *str);
void prv_echo(char c);

__attribute__((noreturn))
void shell_processing_loop(void);


#endif

