// add_collision

.global inc_collision

inc_collision:
.top    
    add r0, r0, #1
    cmp r0, #5
    beg .five
    mov pc, lr
.five
    mov pc, lr    
