#include "console.h"

extern UART_HandleTypeDef huart1;

int shell_putc(char c)
{
  uart_tx_blocking(&c, sizeof(c));
  return 1;
}

void uart_tx_blocking(void * buf, size_t buf_len)
{
	HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)buf_len, ~0);
}

static void prv_log(const char *fmt, va_list *args) 
{
  char log_buf[256];
  const size_t size = vsnprintf(log_buf, sizeof(log_buf) - 2, fmt, *args);
  log_buf[size] = '\r';
  log_buf[size + 1] = '\n';
  uart_tx_blocking(log_buf, size + 2);
}

void logp(const char *fmt, ...) 
{
  va_list args;
  va_start(args, fmt);
  prv_log(fmt, &args);
  va_end(args);
}
