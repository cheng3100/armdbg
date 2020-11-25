#include "usart.h"
#include "shell.h"
#include "dummy.h"
#include "dbg.h"

#define SHELL_RX_BUFFER_SIZE (256)
#define SHELL_MAX_ARGS (16)
#define SHELL_PROMPT "shell> "


#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define SHELL_FOR_EACH_COMMAND(command) \
  for (const sShellCommand *command = g_shell_commands; \
    command < &g_shell_commands[g_num_shell_commands]; \
    ++command)


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

static const sShellCommand s_shell_commands[] = {
  {"bkpt", prv_issue_breakpoint, "Issue a Breakpoint Instruction" },
  {"debug_mon_en", prv_debug_monitor_enable, "Enable Monitor Debug Mode" },
  {"fpb_dump", prv_dump_fpb_config, "Dump Active FPB Settings"},
  {"fpb_set_breakpoint", prv_fpb_set_breakpoint, "Set Breakpoint [Comp Id] [Address]"},
  {"call_dummy_funcs", prv_call_dummy_funcs, "Invoke dummy functions"},
  {"dump_dummy_funcs", prv_dump_dummy_funcs, "Print first instruction of each dummy function"},
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

