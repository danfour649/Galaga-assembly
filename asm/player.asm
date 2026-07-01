;------------------------------------------------------------------------------
; player.asm
;
; Galaga-assembly — player movement, bounds, firing, lives (x86-64, NASM)
;
; History:
;   2026-07-01  Phase 1.2 player_clamp_x, player_tick, player_try_fire
;------------------------------------------------------------------------------

%include "include/galaga.inc"

%define PLAYER_START_X     104    ; GALAGA_WIDTH/2 - 8
%define PLAYER_START_Y     252    ; GALAGA_HEIGHT - 36
%define PLAYER_START_W     16
%define PLAYER_START_H     16

%ifidn __OUTPUT_FORMAT__,win64
    %xdefine ST_PTR rcx
%else
    %xdefine ST_PTR rdi
%endif

section .text

extern entity_spawn

;------------------------------------------------------------------------------
; void player_clamp_x(Player *player)
;------------------------------------------------------------------------------
global player_clamp_x
player_clamp_x:
    mov     eax, [ST_PTR + P_X]
    test    eax, eax
    jns     .not_left
    mov     dword [ST_PTR + P_X], 0
    ret
.not_left:
    mov     edx, [ST_PTR + P_W]
    add     edx, eax
    cmp     edx, GALAGA_WIDTH
    jle     .done
    mov     eax, GALAGA_WIDTH
    sub     eax, [ST_PTR + P_W]
    mov     [ST_PTR + P_X], eax
.done:
    ret

;------------------------------------------------------------------------------
; void player_init(GameState *state)
;------------------------------------------------------------------------------
global player_init
player_init:
    mov     rax, ST_PTR
    mov     dword [rax + P_X], PLAYER_START_X
    mov     dword [rax + P_Y], PLAYER_START_Y
    mov     dword [rax + P_W], PLAYER_START_W
    mov     dword [rax + P_H], PLAYER_START_H
    mov     dword [rax + P_LIVES], PLAYER_START_LIVES
    mov     dword [rax + P_SCORE], 0
    mov     dword [rax + P_STAGE], 1
    ret

;------------------------------------------------------------------------------
; void player_tick(GameState *state, int delta_px, int move_left, int move_right)
;------------------------------------------------------------------------------
global player_tick
player_tick:
    push    rbx
    mov     rbx, ST_PTR
%ifidn __OUTPUT_FORMAT__,win64
    mov     r9d, r9d                     ; move_right
    mov     r8d, r8d                     ; move_left
    mov     r10d, edx                    ; delta_px
%else
    mov     r10d, esi                    ; delta_px
    mov     r8d, edx                     ; move_left
    mov     r9d, ecx                     ; move_right
%endif
    test    r8d, r8d
    jz      .skip_left
    sub     dword [rbx + P_X], r10d
.skip_left:
    test    r9d, r9d
    jz      .clamp
    add     dword [rbx + P_X], r10d
.clamp:
    mov     rdi, rbx
    call    player_clamp_x
    pop     rbx
    ret

;------------------------------------------------------------------------------
; void player_try_fire(GameState *state)
;   Spawns a player bullet; entity_spawn enforces 2-bullet cap.
;------------------------------------------------------------------------------
global player_try_fire
player_try_fire:
    push    rbx
    mov     rbx, ST_PTR
    mov     eax, [rbx + P_X]
    mov     ecx, [rbx + P_W]
    shr     ecx, 1
    add     eax, ecx
    dec     eax                            ; center - 1
    mov     r10d, eax                      ; bullet x
    mov     eax, [rbx + P_Y]
    sub     eax, 8                         ; bullet y
    mov     r11d, eax
%ifidn __OUTPUT_FORMAT__,win64
    mov     rcx, rbx
    mov     edx, ENTITY_KIND_BULLET
    mov     r8d, 1
    mov     r9d, r10d
    push    r11
    sub     rsp, 20h
    call    entity_spawn
    add     rsp, 28h
%else
    mov     rdi, rbx
    mov     esi, ENTITY_KIND_BULLET
    mov     edx, 1
    mov     ecx, r10d
    mov     r8d, r11d
    call    entity_spawn
%endif
    pop     rbx
    ret

;------------------------------------------------------------------------------
; int player_lose_life(GameState *state)
;   Decrements lives; sets game_over when lives reach 0. Returns remaining lives.
;------------------------------------------------------------------------------
global player_lose_life
player_lose_life:
    mov     r10, ST_PTR
    dec     dword [r10 + P_LIVES]
    mov     eax, [r10 + P_LIVES]
    test    eax, eax
    jg      .still_alive
    mov     byte [r10 + GS_GAME_OVER], 1
.still_alive:
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
