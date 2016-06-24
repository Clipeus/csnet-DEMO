# csnet-DEMO
There is demo of the client-server application for Linux in C++ (supports C++14).

The goal of the project is to describe C++ development and socket network programming for Linux with Windows supporting. The demo project describes network programming, cross platform programming (Linux and Windows) and classes model for them in C++14.

There are two console application: a client and a server.
The client sends a request to the server and outputs a result of it.
The server receives a request from the client and sends a response to it.

Use build-all.sh to build all.
Use build-client.sh build a client.
Use build-server.sh build a server.

Now the project can be built for Windows in Visual Studio 2015. Use csnet.sln to do it.

The code can be compiled with Visual C++ 2015 or GCC version 4.9.2, supported 64 bits only.
