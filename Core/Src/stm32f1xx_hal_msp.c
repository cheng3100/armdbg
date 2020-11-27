/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private typedef -----------------------------------------------------------*/
/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* System interrupt init*/

  /** DISABLE: JTAG-DP Disabled and SW-DP Disabled
  */
  /** __HAL_AFIO_REMAP_SWJ_DISABLE(); */
  __HAL_AFIO_REMAP_SWJ_ENABLE();
}
