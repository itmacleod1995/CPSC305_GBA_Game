
.global inc_collision

inc_collision:
.top:    
    add r0, r0, #1
    cmp r0, #5
    bge .five
    b .end
.five:
    mov pc, lr
.end:
    mov pc, lr    
