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
