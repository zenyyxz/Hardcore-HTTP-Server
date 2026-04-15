.intel_syntax noprefix
.section .text
.global _start

_start:
    # Stack layout on entry:
    # [rsp] = argc
    # [rsp+8] = argv[0]
    # [rsp+16] = argv[1]
    # ...
    
    mov rdi, [rsp]      # rdi = argc
    lea rsi, [rsp+8]    # rsi = argv
    
    # Align stack to 16 bytes before calling C++ code
    and rsp, -16
    
    call main
    
    # Exit with the return value of main
    mov rdi, rax
    mov rax, 60         # SYS_exit
    syscall
