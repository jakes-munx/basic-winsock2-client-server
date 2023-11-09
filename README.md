# README

## Introduction
This is a basic C project implementing a network client and server. With the server running, the client establishes a connection andtransfers a hardcoded message. The server Responds by echoing the message back.
This project runs on Windows and uses Winsock2.

## Getting started

### Compiler
The compiler used for this project is GCC. To check if you have GCC install, open a terminal and enter `gcc --version`. If gcc is not recognised, you will need to install the compiler.

Install the GCC compiler by following [these steps](https://www.freecodecamp.org/news/how-to-install-c-and-cpp-compiler-on-windows/)

### Build and Run

Navigate to the server or client folder in cmd and enter `gcc main.c -o out -lws2_32` 
To build the project, run `out.exe`.