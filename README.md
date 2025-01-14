# Minimal Debugger (mdb)
`mdb` is a lightweight debugger designed to demonstrate the inner workings of debugging tools using the `ptrace` syscall. It supports essential debugging capabilities, making it an excellent educational tool for those looking to understand how debuggers function at a low level.

## Features
- Add breakpoints by function symbol or memory address.
- List, delete, and manage breakpoints.
- Step through instructions or continue execution.
- Disassemble code dynamically.

## Running
Clone the repository and change directory:
```sh
git clone https://github.com/kallenosf/minimal-debugger.git
cd minimal-debugger
```
### Running with Docker
1. Build the Docker image:
```sh
docker build -t minimal-debugger .
```
2. Run the container:
```sh
docker run --rm -it minimal-debugger
```
3. Once you have shell access inside the container, run the debugger:
```sh
./mdb [binary_to_debug]
```
All files are copied inside the container, so any binary you want to debug/analyze, must be copied in the project's directory **prior to building and running the container**.
#### Sample binary
There is a tiny sample binary named `debug-me` inside the `binaries` directory. You can use it to test the debugger. It's [source code](https://github.com/kallenosf/minimal-debugger/blob/main/binaries/debug-me.c), `debug-me.c` is also inside the `binaries` directory.
```sh
./mdb binaries/debug-me
```
### Build from source
If you want to compile the debugger from the source code, you need to install two libraries:
- `libelf-dev` : Allows reading and writing of ELF files on a high level
- `libcapstone-dev` : Lightweight multi-architecture disassembly framework
#### Arch linux
```sh
pacman -S libelf
pacman -S capstone
```
#### Ubuntu
```sh
apt install gcc make libelf-dev libcapstone-dev
```
Once you have installed these dependencies:
```sh
make
```
or
```sh
gcc mdb.c -o mdb -lelf -lcapstone
```
Then, you can run the debugger:
```sh
./mdb [binary_to_debug]
```
#### Cleaning Up
To remove the compiled binary and other generated files, run:
```sh
make clean
```
## Usage
To start debugging with mdb, specify the binary you want to debug when launching the program. For example:
```sh
./mdb binaries/debug-me
```
Once loaded, mdb provides a command-line interface with the following commands:
```
./mdb binaries/debug-me

MINIMAL DEBUGGER
====================
Binary loaded: binaries/debug-me
--------------------
Supported Commands:
        1. Add breakpoint command: b <function symbol>
        2. Add breakpoint command: b *<address in hex>
        3. List all existing breakpoints command: l
        4. Delete a breakpoint command: d <b number>
        5. Run the programm command: r
        6. Continue the execution command: c
        7. Execute just a single instruction command: si
        8. Disassemble next 48 bytes command: disas
        9. Disassemble next <n> bytes command: disas <n>
        10. Exit mdb command: q
(mdb) 
```
### Commands and examples
1. Add a Breakpoint by Function Symbol (Sets a breakpoint at the entry point of the main function):
```
b main
```
2. Add a Breakpoint by Memory Address (Sets a breakpoint at the specified address):
```sh
b *0x401000
```
3. List All Existing Breakpoints:
```sh
l
```
4. Delete a Breakpoint:
```sh
d 1
```
5. Continue Execution:
```sh
c
```
6. Single-Step Execution:
```sh
si
```
7. Disassemble Next 48 Bytes:
```sh
disas
```
8. Disassemble Next `n` Bytes:
```sh
disas 64
```
9. Exit the debugger:
```sh
q
```
## Limitations
1. Position-Independent Executables (PIEs):
- Breakpoints by function symbols are unsupported for PIE binaries because the debugger cannot calculate relative addresses at runtime.
- Workaround: Use memory addresses (e.g., b *0x401000) to set breakpoints if you know the absolute address.
2. Dynamic Linking and Symbols:
- `mdb` does not currently support dynamically linked symbols, meaning symbols from shared libraries may not be accessible.
- Workaround: Consider disassembling shared libraries manually or using absolute addresses.
