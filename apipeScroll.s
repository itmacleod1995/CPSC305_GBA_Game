@apipeScroll.s

/* makes the pipes scroll across the screen */

.global apipeScroll
apipeScroll:
    sub r0, r0, #1
    cmp r0, #-1
    blt resetright
    b done   
    resetright:
    mov r0, #240
    b done
    done:
    mov pc, lr
