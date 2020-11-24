#include "usart.h"
#include "shell.h"

#define SHELL_RX_BUFFER_SIZE (256)
#define SHELL_MAX_ARGS (16)
#define SHELL_PROMPT "shell> "


#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define SHELL_FOR_EACH_COMMAND(command) \
  for (const sShellCommand *command = g_shell_commands; \
    command < &g_shell_commands[g_num_shell_commands]; \
    ++command)

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
	// the halting debug bit can only be unset by gdb
  if (prv_halting_debug_enabled()) {
    logp("Halting Debug Enabled - " \
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

static int prv_debug_monitor_enable(int argc, char *argv[]) {
  debug_monitor_enable();
  return 0;
}

static int prv_issue_breakpoint(int argc, char *argv[]) {
  __asm("bkpt 1");
  return 0;
}

static const sShellCommand s_shell_commands[] = {
  {"bkpt", prv_issue_breakpoint, "Issue a Breakpoint Instruction" },
  {"debug_mon_en", prv_debug_monitor_enable, "Enable Monitor Debug Mode" },
  /** {"fpb_dump", prv_dump_fpb_config, "Dump Active FPB Settings"}, */
  /** {"fpb_set_breakpoint", prv_fpb_set_breakpoint, "Set Breakpoint [Comp Id] [Address]"}, */
  /** {"call_dummy_funcs", prv_call_dummy_funcs, "Invoke dummy functions"}, */
  /** {"dump_dummy_funcs", prv_dump_dummy_funcs, "Print first instruction of each dummy function"}, */
  {"help", shell_help_handler, "Lists all commands"},
};

const sShellCommand *const g_shell_commands = s_shell_commands;
const size_t g_num_shell_commands = ARRAY_SIZE(s_shell_commands);

static volatile struct {
  size_t read_idx;
  size_t num_bytes;
  char buf[64];
} s_uart_buffer = {
  .num_bytes = 0,
};

static struct ShellContext {
  int (*send_char)(char c);
  size_t rx_size;
  char rx_buffer[SHELL_RX_BUFFER_SIZE];
} s_shell;

static bool prv_booted(void) {
  return s_shell.send_char != NULL;
}

static void prv_send_char(char c) {
  if (!prv_booted()) {
    return;
  }
  s_shell.send_char(c);
}

static void prv_echo(char c) {
  if ('\n' == c) {
    prv_send_char('\r');
    prv_send_char('\n');
  } else if ('\b' == c || '\x7f' == c) {	// 0x7f backspace
    prv_send_char('\b');
    prv_send_char(' ');
    prv_send_char('\b');
  } else {
    prv_send_char(c);
  }
}

static char prv_last_char(void) {
  return s_shell.rx_buffer[s_shell.rx_size - 1];
}

static bool prv_is_rx_buffer_full(void) {
  return s_shell.rx_size >= SHELL_RX_BUFFER_SIZE;
}

static void prv_reset_rx_buffer(void) {
  memset(s_shell.rx_buffer, 0, sizeof(s_shell.rx_buffer));
  s_shell.rx_size = 0;
}

static void prv_echo_str(const char *str) {
  for (const char *c = str; *c != '\0'; ++c) {
    prv_echo(*c);
  }
}

static void prv_send_prompt(void) {
  prv_echo_str(SHELL_PROMPT);
}

static const sShellCommand *prv_find_command(const char *name) {
  SHELL_FOR_EACH_COMMAND(command) {
    if (strcmp(command->command, name) == 0) {
      return command;
    }
  }
  return NULL;
}

static void prv_process(void)
{
  if (prv_last_char() != '\n' && !prv_is_rx_buffer_full()) {
    return;
  }

  char *argv[SHELL_MAX_ARGS] = {0};
  int argc = 0;

  char *next_arg = NULL;
  for (size_t i = 0; i < s_shell.rx_size && argc < SHELL_MAX_ARGS; ++i) {
    char *const c = &s_shell.rx_buffer[i];
    if (*c == ' ' || *c == '\n' || i == s_shell.rx_size - 1) {
      *c = '\0';
      if (next_arg) {
        argv[argc++] = next_arg;
        next_arg = NULL;
      }
    } else if (!next_arg) {
      next_arg = c;
    }
  }

  if (s_shell.rx_size == SHELL_RX_BUFFER_SIZE) {
    prv_echo('\n');
  }

  if (argc >= 1) {
    const sShellCommand *command = prv_find_command(argv[0]);
    if (!command) {
      prv_echo_str("Unknown command: ");
      prv_echo_str(argv[0]);
      prv_echo('\n');
      prv_echo_str("Type 'help' to list all commands\n");
    } else {
      command->handler(argc, argv);
    }
  }
  prv_reset_rx_buffer();
  prv_send_prompt();
}

void shell_receive_char(char c)
{
  if (c == '\r' || prv_is_rx_buffer_full() || !prv_booted()) {
    return;
  }
  const bool is_backspace = (c == '\b' || c == '\x7f');
  if (is_backspace && s_shell.rx_size == 0) {
    return; // nothing left to delete so don't echo the backspace
  }

  prv_echo(c);

  if (is_backspace) {
    s_shell.rx_buffer[--s_shell.rx_size] = '\0';
    return;
  }

  s_shell.rx_buffer[s_shell.rx_size++] = c;

  prv_process();
}

void shell_put_line(const char *str)
{
  prv_echo_str(str);
  prv_echo('\n');
}


int shell_help_handler(int argc, char *argv[])
{
  SHELL_FOR_EACH_COMMAND(command) {
    prv_echo_str(command->command);
    prv_echo_str(": ");
    prv_echo_str(command->help);
    prv_echo('\n');
  }
  return 0;
}

void uart_tx_blocking(void * buf, size_t buf_len)
{
	HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)buf_len, ~0);
}

static void prv_log(const char *fmt, va_list *args) 
{
  char log_buf[256];
  const size_t size = vsnprintf(log_buf, sizeof(log_buf) - 1, fmt, *args);
  log_buf[size] = '\n';
  uart_tx_blocking(log_buf, size + 1);
}

void logp(const char *fmt, ...) 
{
  va_list args;
  va_start(args, fmt);
  prv_log(fmt, &args);
  va_end(args);
}

static int prv_console_putc(char c)
{
  uart_tx_blocking(&c, sizeof(c));
  return 1;
}

void shell_boot(const sShellImpl *impl)
{
  s_shell.send_char = impl->send_char;
  prv_reset_rx_buffer();
  prv_echo_str("\n" SHELL_PROMPT);
}

void shell_processing_loop(void)
{
  const sShellImpl shell_impl = {
    .send_char = prv_console_putc,
  };
  shell_boot(&shell_impl);

  while (1) {
    char c;
    if (shell_port_getchar(&c)) {
      shell_receive_char(c);
    }
  }
}

void uart_byte_received_cb(uint8_t* buf, uint16_t size)
{
  if (s_uart_buffer.num_bytes >= sizeof(s_uart_buffer.buf)) {
    return; // drop, out of space
  }

  int i=0, j = s_uart_buffer.read_idx;
  while (i<size) {
	s_uart_buffer.buf[j++]= buf[i++];
  }

  s_uart_buffer.num_bytes += size;
}

bool shell_port_getchar(char *c_out) 
{
  if (s_uart_buffer.num_bytes == 0) {
    return false;
  }

  char c = s_uart_buffer.buf[s_uart_buffer.read_idx];

  __disable_irq();
  s_uart_buffer.read_idx = (s_uart_buffer.read_idx + 1) % sizeof(s_uart_buffer.buf);
  s_uart_buffer.num_bytes--;
  __enable_irq();
  *c_out = c;
  return true;
}

