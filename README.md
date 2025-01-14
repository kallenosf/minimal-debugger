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
All files are copied inside the container, so any binary you want to debug/analyze must be copied in the project's directory **prior to building and running the container**.
#### Sample binary
There is a tiny sample binary named `debug-me` inside the `binaries` directory. You can use it to test the debugger. It's [source code](https://github.com/kallenosf/minimal-debugger/blob/main/binaries/debug-me.c), `debug-me.c` is also inside the `binaries` directory.
```sh
./mdb binaries/debug-me
```
### Build from source
If you want to compile the debugger from the source code, you need to install two libraries:
- `libelf-dev` : Allows reading and writing of ELF files on a high level
- `libcapstone-dev` : Lightweight multi-architecture disassembly framework
Once you have installed these dependencies:
```sh
make
```
or
```sh
gcc mdb.c -o mdb -lelf -lcapstone
```
#### Arch linux
```sh
pacman -S libelf
pacman -S capstone
```
#### Ubuntu
```sh
apt install gcc make libelf-dev libcapstone-dev
```
