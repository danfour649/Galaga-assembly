;------------------------------------------------------------------------------
; enemies.asm — formation, entry, wobble, dive AI (x86-64, NASM)
;------------------------------------------------------------------------------

%include "include/galaga.inc"

%ifidn __OUTPUT_FORMAT__,win64
    %xdefine ST_PTR rcx
%else
    %xdefine ST_PTR rdi
%endif

section .text

extern entity_spawn

; row type: 0=boss, 1-2=butterfly, 3-4=bee
row_to_type:
    cmp     eax, 0
    je      .boss
    cmp     eax, 3
    jge     .bee
    mov     eax, ENEMY_TYPE_BUTTERFLY
    ret
.boss:
    mov     eax, ENEMY_TYPE_BOSS
    ret
.bee:
    mov     eax, ENEMY_TYPE_BEE
    ret

setup_entering:
    ; eax = enemy index, rbx = state
    imul    eax, ENEMY_SIZE
    lea     rax, [rbx + rax + GS_ENEMIES]
    mov     dword [rax + E_STATE], ENEMY_STATE_ENTERING
    sub     dword [rax + E_Y], ENTER_DROP
    ret

global enemies_spawn_formation
enemies_spawn_formation:
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    mov     rbx, ST_PTR
    xor     r12d, r12d
.row:
    cmp     r12d, FORM_ROWS
    jge     .done
    xor     r13d, r13d
.col:
    cmp     r13d, FORM_COLS
    jge     .next_row
    mov     eax, r12d
    call    row_to_type
    mov     r15d, eax
    mov     eax, r13d
    imul    eax, FORM_GAP_X
    add     eax, FORM_START_X
    mov     r14d, eax
    mov     eax, r12d
    imul    eax, FORM_GAP_Y
    add     eax, FORM_START_Y
    mov     r10d, eax
%ifidn __OUTPUT_FORMAT__,win64
    mov     rcx, rbx
    xor     edx, edx
    mov     r8d, r15d
    mov     r9d, r14d
    push    r10
    sub     rsp, 20h
    call    entity_spawn
    add     rsp, 28h
%else
    mov     rdi, rbx
    xor     esi, esi
    mov     edx, r15d
    mov     ecx, r14d
    mov     r8d, r10d
    call    entity_spawn
%endif
    cmp     eax, -1
    jl      .next_col
    call    setup_entering
.next_col:
    inc     r13d
    jmp     .col
.next_row:
    inc     r12d
    jmp     .row
.done:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    ret

start_dive:
    push    r12
    mov     r12d, [rbx + P_X]
    mov     ecx, [rax + E_X]
    sub     r12d, ecx
    cmp     dword [rax + E_TYPE], ENEMY_TYPE_BUTTERFLY
    je      .butterfly_vx
    cmp     r12d, 0
    jg      .vx_pos
    mov     r12d, -2
    jmp     .vx_set
.vx_pos:
    mov     r12d, 2
    jmp     .vx_set
.butterfly_vx:
    cmp     r12d, 0
    jg      .bvx_pos
    mov     r12d, -3
    jmp     .vx_set
.bvx_pos:
    mov     r12d, 3
.vx_set:
    mov     dword [rax + E_STATE], ENEMY_STATE_DIVING
    mov     [rax + E_VX], r12d
    mov     dword [rax + E_FIRE_CD], 20
    pop     r12
    ret

try_start_dive:
    ; r14d = enemy index
    mov     eax, r14d
    imul    eax, ENEMY_SIZE
    lea     rax, [rbx + rax + GS_ENEMIES]
    cmp     byte [rax + E_ACTIVE], 0
    je      .no
    cmp     dword [rax + E_STATE], ENEMY_STATE_FORMATION
    jne     .no
    call    start_dive
.no:
    ret

global enemies_tick_all
enemies_tick_all:
    push    rbx
    push    r12
    push    r13
    push    r14
    mov     rbx, ST_PTR
%ifidn __OUTPUT_FORMAT__,win64
    mov     r12d, edx
%else
    mov     r12d, esi
%endif
    mov     r13d, [rbx + GS_FRAME]
    inc     dword [rbx + GS_FRAME]

    ; dive interval scales with stage
    mov     eax, [rbx + P_STAGE]
    imul    eax, 6
    mov     ecx, DIVE_INTERVAL_BASE
    sub     ecx, eax
    cmp     ecx, DIVE_INTERVAL_MIN
    jge     .iv_ok
    mov     ecx, DIVE_INTERVAL_MIN
.iv_ok:
    mov     eax, r13d
    xor     edx, edx
    div     ecx
    test    edx, edx
    jnz     .tick_loop
    mov     r14d, eax
    xor     edx, edx
    mov     eax, r14d
    mov     ecx, MAX_ENEMIES
    div     ecx
    mov     r14d, edx
    call    try_start_dive
    add     r14d, 7
    cmp     r14d, MAX_ENEMIES
    jl      .idx_ok
    sub     r14d, MAX_ENEMIES
.idx_ok:
    call    try_start_dive

.tick_loop:
    xor     r14d, r14d
.each:
    cmp     r14d, MAX_ENEMIES
    jge     .done
    mov     eax, r14d
    imul    eax, ENEMY_SIZE
    lea     rax, [rbx + rax + GS_ENEMIES]
    cmp     byte [rax + E_ACTIVE], 0
    je      .next
    cmp     dword [rax + E_STATE], ENEMY_STATE_ENTERING
    je      .tick_enter
    cmp     dword [rax + E_STATE], ENEMY_STATE_DIVING
    je      .tick_diving
    mov     ecx, r13d
    add     ecx, r14d
    and     ecx, 3
    dec     ecx
    mov     edx, [rax + E_HOME_X]
    add     edx, ecx
    mov     [rax + E_X], edx
    mov     ecx, r13d
    shr     ecx, 2
    add     ecx, r14d
    and     ecx, 1
    mov     edx, [rax + E_HOME_Y]
    add     edx, ecx
    mov     [rax + E_Y], edx
    jmp     .next

.tick_enter:
    mov     ecx, [rax + E_Y]
    add     ecx, r12d
    mov     [rax + E_Y], ecx
    mov     edx, [rax + E_HOME_Y]
    cmp     ecx, edx
    jl      .next
    mov     [rax + E_Y], edx
    mov     dword [rax + E_STATE], ENEMY_STATE_FORMATION
    jmp     .next

.tick_diving:
    mov     ecx, [rax + E_VX]
    add     [rax + E_X], ecx
    mov     ecx, r12d                  ; dive descent 1.5x delta
    shr     ecx, 1
    add     ecx, r12d
    add     [rax + E_Y], ecx
    mov     ecx, [rbx + P_STAGE]
    shr     ecx, 1
    mov     edx, DIVE_FIRE_COOLDOWN
    sub     edx, ecx
    cmp     edx, DIVE_FIRE_CD_MIN
    jge     .fcd_ok
    mov     edx, DIVE_FIRE_CD_MIN
.fcd_ok:
    cmp     dword [rax + E_FIRE_CD], 0
    jg      .cooldown
    push    rax
    push    rdx
    mov     r10d, [rax + E_X]
    mov     r11d, [rax + E_W]
    shr     r11d, 1
    add     r10d, r11d
    dec     r10d
    mov     r11d, [rax + E_Y]
    add     r11d, [rax + E_H]
%ifidn __OUTPUT_FORMAT__,win64
    mov     rcx, rbx
    mov     edx, ENTITY_KIND_BULLET
    xor     r8d, r8d
    mov     r9d, r10d
    push    r11
    sub     rsp, 20h
    call    entity_spawn
    add     rsp, 28h
%else
    mov     rdi, rbx
    mov     esi, ENTITY_KIND_BULLET
    xor     edx, edx
    mov     ecx, r10d
    mov     r8d, r11d
    call    entity_spawn
%endif
    pop     rdx
    pop     rax
    mov     [rax + E_FIRE_CD], edx
    jmp     .return_check
.cooldown:
    dec     dword [rax + E_FIRE_CD]
.return_check:
    mov     ecx, [rax + E_Y]
    cmp     ecx, GALAGA_HEIGHT
    jl      .next
    mov     dword [rax + E_STATE], ENEMY_STATE_FORMATION
    mov     ecx, [rax + E_HOME_X]
    mov     [rax + E_X], ecx
    mov     ecx, [rax + E_HOME_Y]
    mov     [rax + E_Y], ecx
    mov     dword [rax + E_VX], 0
.next:
    inc     r14d
    jmp     .each
.done:
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
