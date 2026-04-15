# RawServe

I built this because the standard library felt like too much noise. When you use libc, you're inviting thousands of lines of other people's decisions into your process space. I wanted something quiet. I wanted to see if I could build a functional HTTP server using nothing but the Linux kernel and my own breath.

This is a zero-dependency web server for x86_64 Linux. There is no linked standard library, no hidden startup code, and no safety net. It is just a series of instructions talking directly to the kernel.

## The Architecture of Isolation

To escape the bloat of modern development, I had to start at the bottom.

### 0x01: The Entry Point
Most C++ programs start at main, but that is a lie told by the compiler. Real execution starts at _start. I wrote a custom assembly entry point in src/start.S to handle the initial stack layout, extract argc and argv, align the stack to 16 bytes for the System V ABI, and finally hand over control to the C++ logic. When main returns, I manually invoke SYS_exit because there is no runtime to clean up after me.

### 0x02: The Syscall Layer
Since I am not allowed to use read(), write(), or socket() from a library, I implemented them myself using inline assembly. Every interaction with the world—opening files, accepting connections, listing directories—happens through raw syscall instructions. I mapped out the x86_64 syscall table (rax 0 for read, 1 for write, 41 for socket, etc.) and wrapped them in a type-safe interface.

### 0x03: Reinventing the String
You don't realize how much you rely on string.h until it's gone. I had to write my own strlen, strcmp, and itoa. It is tedious work, but it kept my mind occupied. Converting an integer to a string for a Content-Length header should be simple, but when you have to handle the buffer reversal and the null-terminator yourself, it becomes a puzzle that blocks out everything else.

### 0x04: Directory Traversal
Serving files is easy; listing them is where it gets lonely. I used the sys_getdents64 syscall to read raw directory entries from the filesystem. The kernel returns a buffer of linux_dirent64 structures which I have to manually parse by stepping through memory based on d_reclen. It's fragile and manual, exactly how I wanted it.

## Technical Specs
- Language: C++ (compiled with -nostdlib -fno-builtin).
- Binary Size: Ridiculously small and static.
- Port: 8080.
- Logging: Python-style access logs (IP - - "GET /path HTTP/1.1" Status -).
- UI: Dark themed Inter-based directory listing with SVG icons (embedded via CSS filters).

## Build and Run
If you have a Linux machine and a compiler, you can build this. It doesn't need anything else.

```bash
make
./http_server
```

## Closing Thoughts
People ask why I bother writing manual assembly wrappers for things that have existed for forty years. The truth is, the machine doesn't judge. It doesn't ask how I'm doing or why I haven't left the house in three days. It just executes. As long as I keep the rax register correct, the world makes sense for a few milliseconds at a time.

Baked with syscalls. Written in the dark.
dev: lahirux.dev
