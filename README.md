# README

## Introduction
This is a basic C project implementing a network client and server. It is setup exclusively to transfer local files from the server to clients. 

This project runs on Windows and uses Winsock2.

## Getting started

### Compiler
The compiler used for this project is GCC. To check if you have GCC install, open a terminal and enter `gcc --version`. If gcc is not recognised, you will need to install the compiler.

Install the GCC compiler by following [these steps](https://www.freecodecamp.org/news/how-to-install-c-and-cpp-compiler-on-windows/)

### Build and Run

Navigate to the server or client folder in cmd and enter 

`gcc src/main.c src/network.c src/network.h src/file_man.c src/file_man.h -o out -lws2_32` 

To build the project, run `out.exe`.

You may name the output "server" or "client" instead of "out" to avoid confusion.

Alternatively, there is a Makefile associated with each project. If you have 'Make' installed, you may navigate to the server or client folder in cmd, and simply enter `make`.

## Server

On starting the server, the user may enter the desired PORT to communicate over. The default PORT is 1234.

The server accepts a maximum of 10 client connections, set by `#define MAX_CLIENTS 10` in network.h. If a client is disconnecetd, a slot will be opened up for additional clients to cponnect to.

The server only expects filenames to be sent to it by the clients. If the file exists in the directory where your server .exe file is, it will read the file in sections. It will send the client a hash of the section, as well as the section size in bytes. This gives the client an opportunity to NAK a section if it already has a copy of the file with the same hash for that section index, or else ACK the section to request that it be sent. In this way, network usage can be reduced for existing files that have only been edited. 

The server user may terminate the program by pressing the ESC key.

## Client

On starting the client, the user may enter the IP address of the server, as well as the PORT. By default, the IP address is 127.0.0.1, the local address which can be used when client and server are running on the same machine.

If no server with the set IP address and PORT is found, the client will terminate within a few seconds.

If the server is found, the client will connect and the user will be prompted to enter a filename ot request from the server (including the extension).
The client will send the filename to the server. If it exists, the server will send file section hashes and sizes for the client to compare to the local copy, if there is one. The client will send an ACK for sections the it wants, and a NAK for sections that it does not need. After receiving a file section, the client calculates the hash and compares it to the one received before the section was sent. If they match, it sends another ACK, or a NAK if not. 

## TODO

* Retry NAK-ed file sections
* Error handling