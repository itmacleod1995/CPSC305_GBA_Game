/* Assembly function that increments collision counter; if r0 == 5, then 5 collisions happened and the bird resets */
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
