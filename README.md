# About This Project

Maybe you are familiar with debugging armchip with a debugger like J-link or Trace32, which can be use to set break points and step through.

But can you image how to do these ==without any debugger or debug interface== ? This project give you a way to do all these just by a uart !

This demo running on a stm32f103rc, but any cortex-based chip is also adaptable by change the link script and replace the hal lib api.

# Getting Started 

## Setting Environment

This project is running at a Linux environment.It is possible porting it to Windows but I don't like the idea. You had better switch to a Linux system.

You should make sure you have already install these tools before start this project at least:

- make
- python3
- [pyocd](https://github.com/pyocd/pyOCD)
- [arm-none-eabi-gcc](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)

There are other tools are recommended:

- picocomm
It is my favorite serial client for Linux. You can install it very easily on Ubuntu by :
`sudo apt install picocomm `

> Tips: Make sure you have add the install path to system path for all the tools!

## Compile

Just type this in the root of project
```shell
make
````

The output file like elf, asm etc can be found at `build` directory.

Also `make clean` will clean up all build result.

## Debug

Connect the St-link or J-link between your board and computer. Then open one terminal, type:

```shell
make gdbserver
```

Then you should see it set up a `3333` gdb port. Then open another terminal, type:

```shell
make gdb
```

Then it will connect to the gdb server and halt stop. You can type the normal gdb command like `c` to continue execute it or `bt` to check the stack frame.

## flash

Connect the St-link or J-link as the same way.Then
```shell
make flash
```
It will compile and load the elf file to the chip flash.

## Start Shell
Just type:
```shell
picocomm /dev/ttyUSB0 -b115200 --omap crcrlf
```
> Note: The `/dev/ttyUSB0` is the uart port on my system. You should replace it with yours.

You can type `help` in the shell to see the available command.
