FROM ubuntu:latest

WORKDIR /app

COPY . .

RUN apt update && apt upgrade -y

RUN apt install gcc make libelf-dev libcapstone-dev -y

RUN gcc -fno-pie -no-pie -o binaries/debug-me binaries/debug-me.c

RUN make