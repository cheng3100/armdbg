#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "dbg.h"
#include "shell.h"
#include "console.h"

static sFpbUnit *const FPB = (sFpbUnit *)0xE0002000;

static eDebugState s_user_requested_debug_state = kDebugState_None;

void debug_monitor_handler_c(sContextStateFrame *frame) {
  volatile uint32_t *demcr = (uint32_t *)0xE000EDFC;

  volatile uint32_t *dfsr = (uint32_t *)0xE000ED30;
  const uint32_t dfsr_dwt_evt_bitmask = (1 << 2);
  const uint32_t dfsr_bkpt_evt_bitmask = (1 << 1);
  const uint32_t dfsr_halt_evt_bitmask = (1 << 0);
  const bool is_dwt_dbg_evt = (*dfsr & dfsr_dwt_evt_bitmask);
  const bool is_bkpt_dbg_evt = (*dfsr & dfsr_bkpt_evt_bitmask);
  const bool is_halt_dbg_evt = (*dfsr & dfsr_halt_evt_bitmask);

  logp("DebugMonitor Exception");

  logp("DEMCR: 0x%08x", *demcr);
  logp("DFSR:  0x%08x (bkpt=%d, halt=%d, dwt=%d)", *dfsr,
              (int)is_bkpt_dbg_evt, (int)is_halt_dbg_evt,
              (int)is_dwt_dbg_evt);

  logp("Register Dump");
  logp(" r0  =0x%08x", frame->r0);
  logp(" r1  =0x%08x", frame->r1);
  logp(" r2  =0x%08x", frame->r2);
  logp(" r3  =0x%08x", frame->r3);
  logp(" r12 =0x%08x", frame->r12);
  logp(" lr  =0x%08x", frame->lr);
  logp(" pc  =0x%08x", frame->return_address);
  logp(" xpsr=0x%08x", frame->xpsr);

  if (is_dwt_dbg_evt || is_bkpt_dbg_evt ||
      (s_user_requested_debug_state == kDebugState_SingleStep))  {
    logp("Debug Event Detected, Awaiting 'c' or 's'");
    while (1) {
      char c;
      if (!shell_getchar(&c)) {
        continue;
      }

      logp("Got char '%c'!\n", c);
      if (c == 'c') { // 'c' == 'continue'
        s_user_requested_debug_state = kDebugState_None;
        break;
      } else if (c == 's') { // 's' == 'single step'
        s_user_requested_debug_state = kDebugState_SingleStep;
        break;
      }
    }
  } else {
    logp("Resuming ...");
  }

  const uint32_t demcr_single_step_mask = (1 << 18);

  if (is_bkpt_dbg_evt) {
    const uint16_t instruction = *(uint16_t*)frame->return_address;
    if ((instruction & 0xff00) == 0xbe00) {
      // advance past breakpoint instruction
      frame->return_address += sizeof(instruction);
    } else {
      // It's a FPB generated breakpoint
      // We need to disable the FPB and single-step
      fpb_disable();
      logp("Single-Stepping over FPB at 0x%x", frame->return_address);
    }

    // single-step to the next instruction
    // This will cause a DebugMonitor interrupt to fire
    // once we return from the exception and a single
    // instruction has been executed. The HALTED bit
    // will be set in the DFSR when this happens.
    *demcr |= (demcr_single_step_mask);
    // We have serviced the breakpoint event so clear mask
    *dfsr = dfsr_bkpt_evt_bitmask;
  } else if (is_halt_dbg_evt) {
    // re-enable FPB in case we got here via single-step
    // for a BKPT debug event
    fpb_enable();

    if (s_user_requested_debug_state != kDebugState_SingleStep) {
      *demcr &= ~(demcr_single_step_mask);
    }

    // We have serviced the single step event so clear mask
    *dfsr = dfsr_halt_evt_bitmask;
  } else if (is_dwt_dbg_evt) {
    // Future exercise: handle DWT debug events
    *dfsr = dfsr_dwt_evt_bitmask;
  }
}

static void prv_enable(bool do_enable) {
  volatile uint32_t *demcr = (uint32_t *)0xE000EDFC;
  const uint32_t mon_en_bit = 16;
  if (do_enable) {
    // clear any stale state in the DFSR
    volatile uint32_t *dfsr = (uint32_t*)0xE000ED30;
    *dfsr = *dfsr;
    *demcr |= 1 << mon_en_bit;
  } else {
    *demcr &= ~(1 << mon_en_bit);
  }
}

static bool prv_halting_debug_enabled(void) {
  volatile uint32_t *dhcsr = (uint32_t *)0xE000EDF0;
  return (((*dhcsr) & 0x1) != 0);
}

bool debug_monitor_enable(void) {
  if (prv_halting_debug_enabled()) {
    logp("Halting Debug Enabled - "
                "Can't Enable Monitor Mode Debug!");
    return false;
  }
  prv_enable(true);


  // Priority for DebugMonitor Exception is bits[7:0].
  // We will use the lowest priority so other ISRs can
  // fire while in the DebugMonitor Interrupt
  volatile uint32_t *shpr3 = (uint32_t *)0xE000ED20;
  *shpr3 = 0xff;

  logp("Monitor Mode Debug Enabled!");
  return true;
}

bool debug_monitor_disable(void) {
  prv_enable(false);
  return true;
}

void fpb_dump_breakpoint_config(void) {
  const uint32_t fp_ctrl = FPB->FP_CTRL;
  const uint32_t fpb_enabled = fp_ctrl & 0x1;
  const uint32_t revision = (fp_ctrl >> 28) & 0xF;
  const uint32_t num_code_comparators =
      (((fp_ctrl >> 12) & 0x7) << 4) | ((fp_ctrl >> 4) & 0xF);

  logp("FPB Revision: %d, Enabled: %d, Hardware Breakpoints: %d",
              revision, (int)fpb_enabled, (int)num_code_comparators);

  for (size_t i = 0; i < num_code_comparators; i++) {
    const uint32_t fp_comp = FPB->FP_COMP[i];
    const bool enabled = fp_comp & 0x1;
    const uint32_t replace = fp_comp >> 30;

    uint32_t instruction_address = fp_comp & 0x1FFFFFFC;
    if (replace == 0x2) {
      instruction_address |= 0x2;
    }

    logp("  FP_COMP[%d] Enabled %d, Replace: %d, Address 0x%x",
                (int)i, (int)enabled, (int)replace, instruction_address);
  }
}


void fpb_disable(void) {
  FPB->FP_CTRL = (FPB->FP_CTRL & ~0x3) | 0x2;
}

void fpb_enable(void) {
  FPB->FP_CTRL |= 0x3;
}

void fpb_get_config(sFpbConfig *config) {
  uint32_t fp_ctrl = FPB->FP_CTRL;

  const uint32_t enabled = fp_ctrl & 0x1;
  const uint32_t revision = (fp_ctrl >> 28) & 0xF;
  const uint32_t num_code =
      (((fp_ctrl >> 12) & 0x7) << 4) | ((fp_ctrl >> 4) & 0xF);
  const uint32_t num_lit = (fp_ctrl >> 8) & 0xF;

  *config = (sFpbConfig) {
    .enabled = enabled != 0,
    .revision = revision,
    .num_code_comparators = num_code,
    .num_literal_comparators = num_lit,
  };
}

bool fpb_set_breakpoint(size_t comp_id, uint32_t instr_addr) {
  sFpbConfig config;
  fpb_get_config(&config);
  if (config.revision != 0) {
    logp("Revision %d Parsing Not Supported", config.revision);
    return false;
  }

  const size_t num_comps = config.num_code_comparators;
  if (comp_id >= num_comps) {
    logp("Instruction Comparator %d Not Implemented", num_comps);
    return false;
  }

  if (instr_addr >= 0x20000000) {
    logp("Address 0x%x is not in code region", instr_addr);
    return false;
  }

  if (!config.enabled) {
    logp("Enabling FPB.");
    fpb_enable();
  }


  const uint32_t replace = (instr_addr & 0x2) == 0 ? 1 : 2;
  const uint32_t fp_comp = (instr_addr & ~0x3) | 0x1 | (replace << 30);
  FPB->FP_COMP[comp_id] = fp_comp;
  return true;
}

bool fpb_get_comp_config(size_t comp_id, sFpbCompConfig *comp_config) {
  sFpbConfig config;
  fpb_get_config(&config);
  if (config.revision != 0) {
    logp("Revision %d Parsing Not Supported", config.revision);
    return false;
  }

  const size_t num_comps = config.num_code_comparators + config.num_literal_comparators;
  if (comp_id >= num_comps) {
    logp("Comparator %d Not Implemented", num_comps);
    return false;
  }

  uint32_t fp_comp = FPB->FP_COMP[comp_id];
  bool enabled = fp_comp & 0x1;
  uint32_t replace = fp_comp >> 30;

  uint32_t address = fp_comp & 0x1FFFFFFC;
  if (replace == 0x2) {
    address |= 0x2;
  }

  *comp_config = (sFpbCompConfig) {
    .enabled = enabled,
    .replace = replace,
    .address = address,
  };
  return true;
}

