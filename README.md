## About The Project

![Demo screen shot](demo.png?raw=true)
This project is intended to act as an introduction to interprocess communication mechanisms in UNIX using sockets.

### The server program:
The user will execute this program using the following syntax:
```sh
./exec_filename port_no
```
where exec_filename is the name of your executable file, and port_no is the port number to create the socket. The port number will be available to the server program as a command-line argument.

The server program does not receive any information from STDIN. Instead, this program receives multiple requests from the client program using sockets. Therefore, the server program creates a child process per request to handle these requests simultaneously. For this reason, the parent process must handle zombine processes by implementing the fireman() call in the primer.

Each child process executes the following tasks:
* First, receive the symbol's information from the client program.
* Next, use the Shannon-Fannon-Elias encoding algorithm to generate the binary code of the symbol.
* Finally, return the binary code to the client program using sockets.

The server program will not print any information to STDOUT.

### The client program:
The user will execute this program using the following syntax:
```sh
./exec_filename hostname port_no < input_filename
```
where exec_filename is the name of your executable file, hostname is the address where the server program is located, port_no is the port number used by the server program, and input_filename is the name of the file with the message (string) to be codified. The hostname and the port number will be available to the client as command-line-arguments.

The client program receives from STDIN a message (string or char array). The input file has a single line with the message.

This program determines the alphabet of the message and the frequency of each symbol in the alphabet, sorting the alphabet in decreasing order based on the frequency (if two or more symbols have the same frequency, they will be sorted using the lexicographic order). Then, it creates m child threads (where m is the size of the alphabet). Each child thread determines the binary code for the assigned symbol executing the following steps:
* Create a socket to communicate with the server program.
* Send the symbol's information to the server program using sockets.
* Wait for the binary code from the server program.
* Write the received information into a memory location accessible by the main thread.