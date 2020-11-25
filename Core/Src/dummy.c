#include "dummy.h"

#define DUMMY_FUNC_ENTRY(_f) \
  { .name = #_f, .func = _f}

const sDummyFunction s_dummy_funcs[] = {
  DUMMY_FUNC_ENTRY(dummy_function_1),
  DUMMY_FUNC_ENTRY(dummy_function_2),
  DUMMY_FUNC_ENTRY(dummy_function_3),
  DUMMY_FUNC_ENTRY(dummy_function_4),
  DUMMY_FUNC_ENTRY(dummy_function_5),
  DUMMY_FUNC_ENTRY(dummy_function_6),
  DUMMY_FUNC_ENTRY(dummy_function_7),
  DUMMY_FUNC_ENTRY(dummy_function_8),
  DUMMY_FUNC_ENTRY(dummy_function_9),
  DUMMY_FUNC_ENTRY(dummy_function_ram),
};

const uint32_t dummy_num = sizeof(s_dummy_funcs)/sizeof(s_dummy_funcs[0]);

void dummy_function_1(void) {
  logp("stub function '%s' called", __func__);
}

void dummy_function_2(void) {
  logp("stub function '%s' called", __func__);
}

void dummy_function_3(void) {
  logp("stub function '%s' called", __func__);
}

void dummy_function_4(void) {
  logp("stub function '%s' called", __func__);
}

void dummy_function_5(void) {
  logp("stub function '%s' called", __func__);
}

void dummy_function_6(void) {
  logp("stub function '%s' called", __func__);
}

void dummy_function_7(void) {
  logp("stub function '%s' called", __func__);
}

void dummy_function_8(void) {
  logp("stub function '%s' called", __func__);
}

void dummy_function_9(void) {
  logp("stub function '%s' called", __func__);
}

__attribute__((section("ram_func")))
void dummy_function_ram(void) {
  logp("stub function '%s' called", __func__);
}
