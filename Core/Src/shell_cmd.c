#include "shell_cmd.h"
#include "main.h"
#include "dummy.h"
#include <stdbool.h>



static int prv_issue_breakpoint(int argc, char *argv[]) {
  __asm("bkpt 1");
  return 0;
}

static int prv_dump_fpb_config(int argc, char *argv[]) {
  fpb_dump_breakpoint_config();
  return 0;
}

static int prv_fpb_set_breakpoint(int argc, char *argv[]) {
  if (argc < 3) {
    logp("Expected [Comp Id] [Address]");
    return -1;
  }

  size_t comp_id = strtoul(argv[1], NULL, 0x0);
  uint32_t addr = strtoul(argv[2], NULL, 0x0);

  bool success = fpb_set_breakpoint(comp_id, addr);
  logp("Set breakpoint on address 0x%x in FP_COMP[%d] %s", addr,
              (int)comp_id, success ? "Succeeded" : "Failed");

  return success ? 0 : -1;
}

static int prv_debug_monitor_enable(int argc, char *argv[]) {
  debug_monitor_enable();
  return 0;
}

static int prv_debug_monitor_disable(int argc, char *argv[]) {
  debug_monitor_disable();
  return 0;
}

static int prv_call_dummy_funcs(int argc, char *argv[]) {
  for (size_t i = 0; i < dummy_num; i++) {
    s_dummy_funcs[i].func();
  }
  return 0;
}

static int prv_dump_dummy_funcs(int argc, char *argv[]) {
  for (size_t i = 0; i < dummy_num; i++) {
    const sDummyFunction *d = &s_dummy_funcs[i];
    // physical address is function address with thumb bit removed
    volatile uint32_t *addr = (uint32_t *)(((uint32_t)d->func) & ~0x1);
    logp("%s: Starts at 0x%x. First Instruction = 0x%x", d->name, addr, *addr);
  }

  return 0;
}

static int shell_help_handler(int argc, char *argv[])
{
  SHELL_FOR_EACH_COMMAND(command) {
    prv_echo_str(command->command);
    prv_echo_str(": ");
    prv_echo_str(command->help);
    prv_echo('\n');
  }
  return 0;
}

static const sShellCommand s_shell_commands[] = {
  {"bkpt", prv_issue_breakpoint, "Issue a Breakpoint Instruction" },
  {"debug_mon_en", prv_debug_monitor_enable, "Enable Monitor Debug Mode" },
  {"debug_mon_off", prv_debug_monitor_disable, "Disable Monitor Debug Mode" },
  {"fpb_dump", prv_dump_fpb_config, "Dump Active FPB Settings"},
  {"fpb_set_breakpoint", prv_fpb_set_breakpoint, "Set Breakpoint [Comp Id] [Address]"},
  {"call_dummy_funcs", prv_call_dummy_funcs, "Invoke dummy functions"},
  {"dump_dummy_funcs", prv_dump_dummy_funcs, "Print first instruction of each dummy function"},
  {"help", shell_help_handler, "Lists all commands"},
};

const sShellCommand *const g_shell_commands = s_shell_commands;
const size_t g_num_shell_commands = ARRAY_SIZE(s_shell_commands);
