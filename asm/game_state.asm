;------------------------------------------------------------------------------
; game_state.asm — title / playing / stage clear / game over (x86-64, NASM)
;------------------------------------------------------------------------------

%include "include/galaga.inc"

%ifidn __OUTPUT_FORMAT__,win64
    %xdefine ST_PTR rcx
%else
    %xdefine ST_PTR rdi
%endif

section .text

extern player_init
extern enemies_spawn_formation

;------------------------------------------------------------------------------
; void game_state_init(GameState *state)
;------------------------------------------------------------------------------
global game_state_init
game_state_init:
    mov     rax, ST_PTR
    mov     dword [rax + GS_PHASE], GAME_PHASE_TITLE
    mov     dword [rax + GS_CLEAR_TIMER], 0
    mov     dword [rax + GS_NEXT_EXTRA_LIFE], EXTRA_LIFE_1
    mov     byte  [rax + GS_GAME_OVER], 0
    ret

;------------------------------------------------------------------------------
; int game_state_is_playing(const GameState *state)
;------------------------------------------------------------------------------
global game_state_is_playing
game_state_is_playing:
    mov     eax, [ST_PTR + GS_PHASE]
    cmp     eax, GAME_PHASE_PLAYING
    sete    al
    movzx   eax, al
    ret

;------------------------------------------------------------------------------
; void game_state_begin_stage_clear(GameState *state)
;------------------------------------------------------------------------------
global game_state_begin_stage_clear
game_state_begin_stage_clear:
    mov     rax, ST_PTR
    mov     dword [rax + GS_PHASE], GAME_PHASE_STAGE_CLEAR
    mov     dword [rax + GS_CLEAR_TIMER], STAGE_CLEAR_FRAMES
    ret

;------------------------------------------------------------------------------
; begin_playing / reset_to_title — internal
;------------------------------------------------------------------------------
begin_playing:
    push    rbx
    mov     rbx, ST_PTR
    mov     dword [rbx + GS_PHASE], GAME_PHASE_PLAYING
    mov     byte  [rbx + GS_GAME_OVER], 0
%ifidn __OUTPUT_FORMAT__,win64
    mov     rcx, rbx
%else
    mov     rdi, rbx
%endif
    call    player_init
%ifidn __OUTPUT_FORMAT__,win64
    mov     rcx, rbx
%else
    mov     rdi, rbx
%endif
    call    enemies_spawn_formation
    pop     rbx
    ret

reset_to_title:
    push    rbx
    mov     rbx, ST_PTR
    mov     dword [rbx + GS_PHASE], GAME_PHASE_TITLE
    mov     byte  [rbx + GS_GAME_OVER], 0
    mov     dword [rbx + GS_CLEAR_TIMER], 0
    mov     dword [rbx + GS_FRAME], 0
%ifidn __OUTPUT_FORMAT__,win64
    mov     rcx, rbx
%else
    mov     rdi, rbx
%endif
    call    player_init
    pop     rbx
    ret

;------------------------------------------------------------------------------
; void game_state_tick(GameState *state, int fire_pressed)
;------------------------------------------------------------------------------
global game_state_tick
game_state_tick:
    mov     rax, ST_PTR
    mov     r8d, [rax + GS_PHASE]  ; scratch reg not used as an arg in either ABI (ST_PTR clobbering RCX broke win64)
    cmp     r8d, GAME_PHASE_TITLE
    je      .title
    cmp     r8d, GAME_PHASE_STAGE_CLEAR
    je      .stage_clear
    cmp     r8d, GAME_PHASE_GAME_OVER
    je      .game_over
    ret
.title:
%ifidn __OUTPUT_FORMAT__,win64
    test    edx, edx
%else
    test    esi, esi
%endif
    jz      .out
    jmp     begin_playing
.stage_clear:
    cmp     dword [rax + GS_CLEAR_TIMER], 0
    jle     .advance_stage
    dec     dword [rax + GS_CLEAR_TIMER]
    ret
.advance_stage:
    inc     dword [rax + P_STAGE]
    mov     dword [rax + GS_PHASE], GAME_PHASE_PLAYING
%ifidn __OUTPUT_FORMAT__,win64
    mov     rcx, ST_PTR
%else
    mov     rdi, ST_PTR
%endif
    call    enemies_spawn_formation
    ret
.game_over:
%ifidn __OUTPUT_FORMAT__,win64
    test    edx, edx
%else
    test    esi, esi
%endif
    jz      .out
    jmp     reset_to_title
.out:
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
