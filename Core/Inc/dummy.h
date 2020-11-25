#pragma once

#include <stdint.h>

typedef struct {
  const char *name;
  void (*func)(void);
} sDummyFunction;

extern const sDummyFunction s_dummy_funcs[];
const uint32_t dummy_num;

void logp(const char *fmt, ...);

void dummy_function_1(void);
void dummy_function_2(void);
void dummy_function_3(void);
void dummy_function_4(void);
void dummy_function_5(void);
void dummy_function_6(void);
void dummy_function_7(void);
void dummy_function_8(void);
void dummy_function_9(void);
void dummy_function_ram(void);
