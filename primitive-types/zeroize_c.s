#
# zeroize_c.s
#
# C style zeroizing - in batches
#
    .global  _start

    .text
_start:
    
    # Make a loop which will keep calling zeroize
    mov $1000000, %r15
z_loop:    
    mov $100000, %rdi   # Buffer size in bytes
    call zeroize
    dec %r15
    cmp $0, %r15
    jnz z_loop

exit:
    mov $60, %rax       # exit's system call number
    mov $0, %rbx        # exit(0)
    syscall

zeroize:
    push %rbp
    mov %rsp, %rbp
    sub %rdi, %rsp      # Allocate memory on the stack
    
    # memset style zeroization code
    mov $0, %rax        # Zeroize
    mov $12500, %rcx    # Number of iterations = number of bytes/8
    mov %rsp, %rdi      # Move the starting address into rdi
    rep stosq           # 8-bytes/quad-word at a time
    
    leave
    ret

