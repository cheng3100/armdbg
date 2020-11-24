/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_it.h"
#include "shell.h"
#include "fpb.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart1;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

typedef struct __attribute__((packed)) ContextStateFrame {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  uint32_t lr;
  uint32_t return_address;
  uint32_t xpsr;
} sContextStateFrame;

typedef enum {
  kDebugState_None,
  kDebugState_SingleStep,
} eDebugState;

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
      if (!shell_port_getchar(&c)) {
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
/**
  * @brief This function handles Debug monitor.
  */
__attribute__((naked))
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */
  __asm volatile(
      "tst lr, #4 \n"
      "ite eq \n"
      "mrseq r0, msp \n"
      "mrsne r0, psp \n"
      "b debug_monitor_handler_c \n");
  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
