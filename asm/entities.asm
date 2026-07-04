;------------------------------------------------------------------------------
; entities.asm
;
; Galaga-assembly — fixed-size enemy and bullet pools (x86-64, NASM)
;
; History:
;   2026-07-01  Phase 1.1 entity_spawn, entity_kill, entity_tick_all
;------------------------------------------------------------------------------

%include "include/galaga.inc"

%ifidn __OUTPUT_FORMAT__,win64
    %xdefine ST_PTR  rcx
    %xdefine ARG_KIND edx
    %xdefine ARG_TYPE r8d
    %xdefine ARG_X    r9d
%else
    %xdefine ST_PTR  rdi
    %xdefine ARG_KIND esi
    %xdefine ARG_TYPE edx
    %xdefine ARG_X    ecx
%endif

section .text

;------------------------------------------------------------------------------
; int entity_spawn(GameState *state, int kind, int type, int x, int y)
;------------------------------------------------------------------------------
global entity_spawn
entity_spawn:
%ifidn __OUTPUT_FORMAT__,win64
    mov     r11d, dword [rsp + 28h]    ; y (before callee push)
%endif
    push    rbx
    mov     rbx, ST_PTR
%ifnidn __OUTPUT_FORMAT__,win64
    mov     r11d, r8d                  ; y
%endif

    cmp     ARG_KIND, ENTITY_KIND_BULLET
    je      .spawn_bullet
    cmp     ARG_KIND, ENTITY_KIND_ENEMY
    jne     .fail

    ; --- enemy ---
    xor     ecx, ecx
.enemy_find:
    cmp     ecx, MAX_ENEMIES
    jge     .fail
    mov     eax, ecx
    imul    eax, ENEMY_SIZE
    lea     rax, [rbx + rax + GS_ENEMIES]
    cmp     byte [rax + E_ACTIVE], 0
    je      .enemy_init
    inc     ecx
    jmp     .enemy_find
.enemy_init:
    mov     byte [rax + E_ACTIVE], 1
    mov     [rax + E_X], ARG_X
    mov     [rax + E_Y], r11d
    mov     dword [rax + E_W], ENEMY_W_DEFAULT
    mov     dword [rax + E_H], ENEMY_H_DEFAULT
    mov     [rax + E_TYPE], ARG_TYPE
    mov     dword [rax + E_STATE], ENEMY_STATE_FORMATION
    mov     [rax + E_HOME_X], ARG_X
    mov     [rax + E_HOME_Y], r11d
    mov     dword [rax + E_VX], 0
    mov     dword [rax + E_FIRE_CD], 0
    cmp     ARG_TYPE, ENEMY_TYPE_BOSS
    je      .hp_boss
    mov     dword [rax + E_HP], 1
    jmp     .hp_done
.hp_boss:
    mov     dword [rax + E_HP], 2
.hp_done:
    mov     eax, ecx
    pop     rbx
    ret

.spawn_bullet:
    cmp     ARG_TYPE, 1
    jne     .bullet_find
    xor     ecx, ecx
    xor     edx, edx
.count_player:
    cmp     ecx, MAX_BULLETS
    jge     .bullet_find
    mov     eax, ecx
    imul    eax, BULLET_SIZE
    lea     rax, [rbx + rax + GS_BULLETS]
    cmp     byte [rax + B_ACTIVE], 0
    je      .count_next
    cmp     byte [rax + B_FROM_PLAYER], 0
    je      .count_next
    inc     edx
.count_next:
    inc     ecx
    jmp     .count_player
.bullet_find:
    cmp     ARG_TYPE, 1
    jne     .bullet_slot
    cmp     edx, MAX_PLAYER_BULLETS
    jge     .fail
.bullet_slot:
    xor     ecx, ecx
.bullet_find_loop:
    cmp     ecx, MAX_BULLETS
    jge     .fail
    mov     eax, ecx
    imul    eax, BULLET_SIZE
    lea     rax, [rbx + rax + GS_BULLETS]
    cmp     byte [rax + B_ACTIVE], 0
    je      .bullet_init
    inc     ecx
    jmp     .bullet_find_loop
.bullet_init:
    mov     byte [rax + B_ACTIVE], 1
    mov     [rax + B_X], ARG_X
    mov     [rax + B_Y], r11d
    mov     dword [rax + B_W], BULLET_W_DEFAULT
    mov     dword [rax + B_H], BULLET_H_DEFAULT
%ifidn __OUTPUT_FORMAT__,win64
    mov     byte [rax + B_FROM_PLAYER], r8b
%else
    mov     byte [rax + B_FROM_PLAYER], dl
%endif
    mov     eax, ecx
    pop     rbx
    ret

.fail:
    mov     eax, -1
    pop     rbx
    ret

;------------------------------------------------------------------------------
; void entity_kill(GameState *state, int kind, int index)
;------------------------------------------------------------------------------
global entity_kill
entity_kill:
    mov     r10, ST_PTR
%ifidn __OUTPUT_FORMAT__,win64
    mov     r11d, r8d
%else
    mov     r11d, edx
%endif
    cmp     ARG_KIND, ENTITY_KIND_BULLET
    je      .kill_bullet
    cmp     ARG_KIND, ENTITY_KIND_ENEMY
    jne     .kill_ret
    cmp     r11d, MAX_ENEMIES
    jge     .kill_ret
    mov     eax, r11d
    imul    rax, ENEMY_SIZE
    add     rax, GS_ENEMIES
    add     rax, r10
    mov     byte [rax + E_ACTIVE], 0
    ret
.kill_bullet:
    cmp     r11d, MAX_BULLETS
    jge     .kill_ret
    mov     eax, r11d
    imul    rax, BULLET_SIZE
    add     rax, GS_BULLETS
    add     rax, r10
    mov     byte [rax + B_ACTIVE], 0
.kill_ret:
    ret

;------------------------------------------------------------------------------
; void entity_tick_all(GameState *state, int delta_px)
;------------------------------------------------------------------------------
global entity_tick_all
entity_tick_all:
    push    rbx
    mov     rbx, ST_PTR
%ifidn __OUTPUT_FORMAT__,win64
    mov     r9d, edx
%else
    mov     r9d, esi
%endif
    xor     ecx, ecx
.tick_loop:
    cmp     ecx, MAX_BULLETS
    jge     .tick_done
    mov     eax, ecx
    imul    eax, BULLET_SIZE
    lea     rax, [rbx + rax + GS_BULLETS]
    cmp     byte [rax + B_ACTIVE], 0
    je      .tick_next
    cmp     byte [rax + B_FROM_PLAYER], 0
    je      .tick_down
    lea     r10d, [r9d + r9d*2]        ; player bullets travel 3x delta
    sub     dword [rax + B_Y], r10d
    jmp     .tick_bounds
.tick_down:
    mov     r10d, r9d                  ; enemy bullets travel 1.5x delta
    shr     r10d, 1
    add     r10d, r9d
    add     dword [rax + B_Y], r10d
.tick_bounds:
    mov     r10d, [rax + B_Y]
    mov     r11d, [rax + B_H]
    neg     r11d
    cmp     r10d, r11d
    jl      .tick_off
    cmp     r10d, GALAGA_HEIGHT
    jg      .tick_off
    jmp     .tick_next
.tick_off:
    mov     byte [rax + B_ACTIVE], 0
.tick_next:
    inc     ecx
    jmp     .tick_loop
.tick_done:
    pop     rbx
    ret

;------------------------------------------------------------------------------
; int entity_any_active(GameState *state, int kind)
;------------------------------------------------------------------------------
global entity_any_active
entity_any_active:
    mov     r10, ST_PTR
    cmp     ARG_KIND, ENTITY_KIND_BULLET
    je      .any_bullet
    cmp     ARG_KIND, ENTITY_KIND_ENEMY
    jne     .any_no
    xor     ecx, ecx
.any_enemy:
    cmp     ecx, MAX_ENEMIES
    jge     .any_no
    mov     eax, ecx
    imul    eax, ENEMY_SIZE
    lea     rax, [r10 + rax + GS_ENEMIES]
    cmp     byte [rax + E_ACTIVE], 0
    jne     .any_yes
    inc     ecx
    jmp     .any_enemy
.any_bullet:
    xor     ecx, ecx
.any_bull:
    cmp     ecx, MAX_BULLETS
    jge     .any_no
    mov     eax, ecx
    imul    eax, BULLET_SIZE
    lea     rax, [r10 + rax + GS_BULLETS]
    cmp     byte [rax + B_ACTIVE], 0
    jne     .any_yes
    inc     ecx
    jmp     .any_bull
.any_yes:
    mov     eax, 1
    ret
.any_no:
    xor     eax, eax
    ret

;------------------------------------------------------------------------------
; void entities_spawn_demo_formation(GameState *state)
;------------------------------------------------------------------------------
global entities_spawn_demo_formation
entities_spawn_demo_formation:
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    mov     rbx, ST_PTR
    xor     r12d, r12d
.row:
    cmp     r12d, 3
    jge     .done
    xor     r13d, r13d
.col:
    cmp     r13d, 8
    jge     .next_row
    mov     eax, r13d
    imul    eax, 22
    add     eax, 24              ; x
    mov     r14d, eax
    mov     eax, r12d
    imul    eax, 20
    add     eax, 40              ; y in r15d
    mov     r15d, eax
%ifidn __OUTPUT_FORMAT__,win64
    mov     rcx, rbx
    xor     edx, edx
    mov     r8d, r12d
    mov     r9d, r14d
    push    r15
    sub     rsp, 20h
    call    entity_spawn
    add     rsp, 28h
%else
    mov     rdi, rbx
    xor     esi, esi
    mov     edx, r12d
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

section .note.GNU-stack noalloc noexec nowrite progbits
