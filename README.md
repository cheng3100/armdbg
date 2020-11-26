# armdbg

implement single step debug on arm cortex core without a gdb or Trace32

This project is a minmal demo shwo how to do a single step, set break point using just the uart monitor, without any use of gdb or Trace32.

This demo running on a stm32f103rc, but any cortex-based chip is also adaptable by change the link script and replace the hal lib api.
