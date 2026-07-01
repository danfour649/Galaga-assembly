;------------------------------------------------------------------------------
; score.asm — scoring and extra lives (x86-64, NASM)
;------------------------------------------------------------------------------

%include "include/galaga.inc"

%ifidn __OUTPUT_FORMAT__,win64
    %xdefine ST_PTR rcx
%else
    %xdefine ST_PTR rdi
%endif

section .text

global score_add_points
score_add_points:
    push    rbx
    mov     rbx, ST_PTR
%ifidn __OUTPUT_FORMAT__,win64
    add     [rbx + P_SCORE], edx
%else
    add     [rbx + P_SCORE], esi
%endif
.check_extra:
    mov     eax, [rbx + GS_NEXT_EXTRA_LIFE]
    cmp     eax, 0
    jle     .done
    mov     ecx, [rbx + P_SCORE]
    cmp     ecx, eax
    jl      .done
    inc     dword [rbx + P_LIVES]
    cmp     eax, EXTRA_LIFE_1
    jne     .not_first
    mov     dword [rbx + GS_NEXT_EXTRA_LIFE], EXTRA_LIFE_2
    jmp     .check_extra
.not_first:
    cmp     eax, EXTRA_LIFE_2
    jne     .not_second
    mov     dword [rbx + GS_NEXT_EXTRA_LIFE], EXTRA_LIFE_3
    jmp     .check_extra
.not_second:
    mov     dword [rbx + GS_NEXT_EXTRA_LIFE], 0
.done:
    pop     rbx
    ret

;------------------------------------------------------------------------------
; void score_add_enemy_kill(GameState *state, int enemy_index)
;------------------------------------------------------------------------------
global score_add_enemy_kill
score_add_enemy_kill:
    push    rbx
    mov     rbx, ST_PTR
%ifidn __OUTPUT_FORMAT__,win64
    mov     r8d, edx
%else
    mov     r8d, esi
%endif
    mov     eax, r8d
    imul    eax, ENEMY_SIZE
    lea     rax, [rbx + rax + GS_ENEMIES]
    mov     edx, SCORE_BEE_FORMATION
    cmp     dword [rax + E_TYPE], ENEMY_TYPE_BUTTERFLY
    je      .butterfly
    cmp     dword [rax + E_TYPE], ENEMY_TYPE_BOSS
    je      .boss
    cmp     dword [rax + E_STATE], ENEMY_STATE_DIVING
    jne     .add
    mov     edx, SCORE_BEE_DIVING
    jmp     .add
.butterfly:
    mov     edx, SCORE_BUTTERFLY_FORMATION
    cmp     dword [rax + E_STATE], ENEMY_STATE_DIVING
    jne     .add
    mov     edx, SCORE_BUTTERFLY_DIVING
    jmp     .add
.boss:
    mov     edx, SCORE_BOSS_FORMATION
    cmp     dword [rax + E_STATE], ENEMY_STATE_DIVING
    jne     .add
    mov     edx, SCORE_BOSS_DIVING
.add:
%ifidn __OUTPUT_FORMAT__,win64
    mov     rcx, rbx
    call    score_add_points
%else
    mov     rdi, rbx
    mov     esi, edx
    call    score_add_points
%endif
    pop     rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
