# RawServe

This project exists because of a dare. My girlfriend bet me I couldn't build a functional HTTP server without touching a single standard library. No libc, no crt0, no safety nets. Just raw x86_64 Linux syscalls and some manual byte-pushing.

She lost the bet. This is the result: a zero-dependency, statically-linked web server that talks directly to the Linux kernel.

## The Absolute Zero Flex

Most "minimal" projects still lean on the C standard library. RawServe does not. It uses `-nostdlib` to ensure that not a single byte of outside code is linked into the final binary.

### 0x01: Taking Control of _start
We don't use the standard entry point. Execution begins in `src/start.S`, a custom assembly file that manually sets up the stack according to the System V ABI. It extracts `argc` and `argv`, aligns the stack to 16 bytes, and hands off to the C++ logic. When the server shuts down, we don't return; we manually invoke `SYS_exit` because there is no runtime to clean up for us.

### 0x02: The Syscall Engine
Since `read()`, `write()`, and `socket()` are off-limits, I implemented them using inline assembly. Every operation—binding sockets, accepting connections, reading files—is a direct conversation with the kernel via the `syscall` instruction. I mapped the x86_64 syscall table by hand to create a type-safe interface for everything from networking to filesystem polling.

### 0x03: Building the Basics from Scratch
When you delete libc, you delete `string.h`. I had to write my own implementations of `strlen`, `strcmp`, `memcpy`, and a custom `itoa` for generating HTTP headers. Even basic tasks like parsing an IP address or formatting a `Content-Length` header required building the logic from the ground up.

### 0x04: Manual ZIP Streaming
To support directory downloads, I implemented the ZIP file format specification from scratch. RawServe generates "Stored" (uncompressed) ZIP archives on-the-fly. It manually constructs the Local File Headers and Central Directory records, streaming them directly to the socket. I even had to implement a custom CRC32 algorithm to ensure archive integrity. No `zlib`, no `libzip`. Just bytes.

## Features
- Zero dependencies. No libc, no headers beyond the kernel interface.
- Custom assembly entry point for x86_64 Linux.
- Static file serving with MIME type detection.
- Dynamic HTML directory listings with embedded SVG icons.
- On-the-fly ZIP streaming for directory downloads.
- High-performance, synchronous I/O.

## Build and Run
The build process is as minimal as the code. You only need a compiler and `make`.

```bash
make
./http_server -p 8080 -d ./public
```

### Options
- **-p <port>**: Port to listen on (default: 8080).
- **-d <dir>**: Root directory to serve.
- **-i <addr>**: IP address to bind to.
- **-q**: Quiet mode (no logging).
- **-n**: Disable ZIP streaming.
- **-v**: Version information.

## Technical Summary
This isn't just a web server; it's an exercise in system-level autonomy. By bypassing the standard library, we eliminate the hidden overhead of generic runtimes and gain absolute control over every register and memory address in the process.

Special thanks to Sophie :) for the dare.
