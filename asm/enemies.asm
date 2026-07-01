;------------------------------------------------------------------------------
; enemies.asm
;
; Galaga-assembly — bee formation, wobble, dive attacks (x86-64, NASM)
;
; History:
;   2026-07-01  Phase 1.3 formation, wobble, dive, enemy fire
;------------------------------------------------------------------------------

%include "include/galaga.inc"

%ifidn __OUTPUT_FORMAT__,win64
    %xdefine ST_PTR rcx
%else
    %xdefine ST_PTR rdi
%endif

section .text

extern entity_spawn

;------------------------------------------------------------------------------
; void enemies_spawn_formation(GameState *state)
;   Spawns 5×8 bees in a static grid.
;------------------------------------------------------------------------------
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
    mov     eax, r13d
    imul    eax, FORM_GAP_X
    add     eax, FORM_START_X
    mov     r14d, eax
    mov     eax, r12d
    imul    eax, FORM_GAP_Y
    add     eax, FORM_START_Y
    mov     r15d, eax
%ifidn __OUTPUT_FORMAT__,win64
    mov     rcx, rbx
    xor     edx, edx
    mov     r8d, ENEMY_TYPE_BEE
    mov     r9d, r14d
    push    r15
    sub     rsp, 20h
    call    entity_spawn
    add     rsp, 28h
%else
    mov     rdi, rbx
    xor     esi, esi
    mov     edx, ENEMY_TYPE_BEE
    mov     ecx, r14d
    mov     r8d, r15d
    call    entity_spawn
%endif
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

;------------------------------------------------------------------------------
; Start dive for enemy at rax (pointer to enemy struct)
; rbx = state
;------------------------------------------------------------------------------
start_dive:
    push    r12
    mov     r12d, [rbx + P_X]
    mov     ecx, [rax + E_X]
    sub     r12d, ecx
    cmp     r12d, 0
    jg      .vx_pos
    mov     r12d, -2
    jmp     .vx_set
.vx_pos:
    cmp     r12d, 0
    jl      .vx_neg
    mov     r12d, 2
    jmp     .vx_set
.vx_neg:
    mov     r12d, -2
.vx_set:
    mov     dword [rax + E_STATE], ENEMY_STATE_DIVING
    mov     [rax + E_VX], r12d
    mov     dword [rax + E_FIRE_CD], 20
    pop     r12
    ret

;------------------------------------------------------------------------------
; void enemies_tick_all(GameState *state, int delta_px)
;------------------------------------------------------------------------------
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

    ; Try to start a dive periodically
    mov     eax, r13d
    xor     edx, edx
    mov     ecx, DIVE_INTERVAL
    div     ecx
    test    edx, edx
    jnz     .tick_loop
    mov     eax, r13d
    mov     ecx, MAX_ENEMIES
    div     ecx
    mov     r14d, edx
    mov     eax, r14d
    imul    eax, ENEMY_SIZE
    lea     rax, [rbx + rax + GS_ENEMIES]
    cmp     byte [rax + E_ACTIVE], 0
    je      .tick_loop
    cmp     dword [rax + E_STATE], ENEMY_STATE_FORMATION
    jne     .tick_loop
    call    start_dive

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
    cmp     dword [rax + E_STATE], ENEMY_STATE_DIVING
    je      .tick_diving
    ; formation wobble
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

.tick_diving:
    mov     ecx, [rax + E_VX]
    add     [rax + E_X], ecx
    add     [rax + E_Y], r12d
    cmp     dword [rax + E_FIRE_CD], 0
    jg      .cooldown
    push    rax
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
    pop     rax
    mov     dword [rax + E_FIRE_CD], DIVE_FIRE_COOLDOWN
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
