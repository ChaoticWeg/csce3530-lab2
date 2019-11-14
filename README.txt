Shawn Lutch - sml0262
CSCE 3530.001 Lab 3


NOTE: a GNU Makefile has been provided (the usage of which is documented below). CMakeLists have also been provided,
both for the executables and for the shared binaries (I needed them for development using CLion). These are only
necessary if you need them; for building on the CSE machines, the Makefile is plenty, and CMakeLists can be ignored.


What's in the box:

    Along with this README.txt, there is a Makefile, a client.c and server.c, and some shared source code found in
    src/. The file structure is as follows:

    +  .
    |
    +  src/
    |  +  common.h  -- Shared helper functions
    |  +  common.c  -- Implementation of common.h
    |  +  mytcp.h   -- Shared mytcp_t struct (represents TCP header) and helper functions related to TCP headers
    |  +  mytcp.c   -- Implementation of mytcp.h
    |
    +  client.c     -- Client logic
    +  server.c     -- Server logic
    +  Makefile     -- Rules and recipes for building libraries and binaries

    See instructions below for how to build. Execution logic is explained below in "How it works."


How to build:

    Build client and server executables, along with shared libraries:
        $ make

    Alternatively, if you like typing more letters, build everything with this:
        $ make all

    ...or build just one of the two, if you want:
        $ make client
        $ make server

    Build the shared libraries by themselves if you want, I'm not a cop:
        $ make libraries

    Clean executables and shared libraries:
        $ make clean

    Want to build a fresh copy but don't have time to type "make clean && make all"? I got you.
        $ make fresh


How to run:

    NOTE: program output is written to stdout, while TCP segments are written to stderr. Segments can be separated from
    regular output by redirecting stderr. The following commands call the program within the specifications of the
    assignment.

    The program takes one argument: "open" or "close". This argument will be referred to as ACTION below.

    First, of course, build the executables:
        $ make

    Run the server before the client, on CSE01:
        $ ./server ACTION 2> server.out

    Then run the client, on any other CSE machine:
        $ ./client ACTION 2> client.out

    ACTION must be the same for client as it is for server; if not, one side will error out because proper protocol
    will have been broken. On a related note, this is a great way to test whether protocol validation works!

    The server will close once execution completes.


How it works:

    The struct mytcp (and its typedef, mytcp_t) are defined in src/mytcp.h and outline the structure of a TCP IP
    header. The other utility and helper functions associated with this struct are declared in this header file.

    As a side note, the struct mytcp is defined with __attribute__((packed)), ensuring that the struct has the same size
    on all processors without fields being padded. This was necessary during development, because I used Cygwin and
    CMake on my Windows development machine while running the client from here, and used the toolchain provided on
    CSE01 to build and run the server. This is likely not necessary when running both sides on CSE machines, but I've
    left it in anyway.

    Instead of passing an array of characters to the read/write system calls as a buffer, we instead pass the address
    of one of these structs. Since we can ensure the struct is understood and communicated properly between client and
    server, setting and verifying fields is trivial.
