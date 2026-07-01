;------------------------------------------------------------------------------
; collision.asm — AABB overlap and collision resolution (x86-64, NASM)
;------------------------------------------------------------------------------

%include "include/galaga.inc"

%ifidn __OUTPUT_FORMAT__,win64
    %xdefine A_PTR rcx
    %xdefine B_PTR rdx
%else
    %xdefine A_PTR rdi
    %xdefine B_PTR rsi
%endif

section .text

extern entity_kill
extern player_lose_life
extern score_add_enemy_kill

;------------------------------------------------------------------------------
; int rect_overlap(const Rect *a, const Rect *b)
;------------------------------------------------------------------------------
global rect_overlap
rect_overlap:
    mov  eax, [A_PTR + E_X]
    mov  ecx, [A_PTR + E_Y]
    mov  edx, [A_PTR + E_W]
    mov  r8d, [A_PTR + E_H]
    mov  r9d, [B_PTR + E_X]
    mov  r10d, [B_PTR + E_W]
    add  r10d, r9d
    cmp  eax, r10d
    jge  .no_overlap
    lea  r10d, [rax + rdx]
    cmp  r9d, r10d
    jge  .no_overlap
    mov  eax, [B_PTR + E_Y]
    mov  edx, [B_PTR + E_H]
    lea  r10d, [rax + rdx]
    cmp  ecx, r10d
    jge  .no_overlap
    mov  edx, [A_PTR + E_H]
    lea  r10d, [rcx + rdx]
    cmp  eax, r10d
    jge  .no_overlap
    mov  eax, 1
    ret
.no_overlap:
    xor  eax, eax
    ret

;------------------------------------------------------------------------------
; overlap_boxes_at: rax=box a, r11=box b → eax=1/0 (clobbers ecx, edx, r8d, r9d, r10d)
;------------------------------------------------------------------------------
overlap_boxes_at:
    push    r10
    mov     r10, rax
    mov     eax, [r10 + E_X]
    mov     ecx, [r10 + E_Y]
    mov     edx, [r10 + E_W]
    mov     r8d, [r10 + E_H]
    mov     r9d, [r11 + E_X]
    mov     eax, [r11 + E_W]
    add     eax, r9d
    cmp     dword [r10 + E_X], eax
    jge     .obox_no
    mov     eax, [r10 + E_X]
    add     eax, [r10 + E_W]
    cmp     dword [r11 + E_X], eax
    jge     .obox_no
    mov     eax, [r11 + E_Y]
    add     eax, [r11 + E_H]
    cmp     ecx, eax
    jge     .obox_no
    mov     eax, [r10 + E_Y]
    add     eax, r8d
    cmp     dword [r11 + E_Y], eax
    jge     .obox_no
    mov     eax, 1
    pop     r10
    ret
.obox_no:
    xor     eax, eax
    pop     r10
    ret

;------------------------------------------------------------------------------
; record_hit: r15=hits, eax=type, ebx=bullet_idx, ecx=enemy_idx
;------------------------------------------------------------------------------
%macro RECORD_HIT 0
    mov     edx, [r15 + H_OFF_COUNT]
    cmp     edx, MAX_COLLISION_HITS
    jge     %%skip_rec
    mov     [r15 + H_OFF_TYPE + rdx * 4], eax
    mov     [r15 + H_OFF_BULLET + rdx * 4], ebx
    mov     [r15 + H_OFF_ENEMY + rdx * 4], ecx
    inc     dword [r15 + H_OFF_COUNT]
%%skip_rec:
%endmacro

;------------------------------------------------------------------------------
; void collision_resolve(GameState *state, CollisionHits *hits)
;------------------------------------------------------------------------------
global collision_resolve
collision_resolve:
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
%ifidn __OUTPUT_FORMAT__,win64
    mov     rbx, rcx
    mov     r15, rdx
%else
    mov     rbx, rdi
    mov     r15, rsi
%endif
    mov     dword [r15 + H_OFF_COUNT], 0

    ; --- player bullets vs enemies ---
    xor     r12d, r12d
.pb_bullet:
    cmp     r12d, MAX_BULLETS
    jge     .enemy_bullets
    mov     eax, r12d
    imul    eax, BULLET_SIZE
    lea     r13, [rbx + rax + GS_BULLETS]
    cmp     byte [r13 + B_ACTIVE], 0
    je      .pb_next_b
    cmp     byte [r13 + B_FROM_PLAYER], 0
    je      .pb_next_b
    xor     r14d, r14d
.pb_enemy:
    cmp     r14d, MAX_ENEMIES
    jge     .pb_next_b
    mov     eax, r14d
    imul    eax, ENEMY_SIZE
    lea     rax, [rbx + rax + GS_ENEMIES]
    cmp     byte [rax + E_ACTIVE], 0
    je      .pb_next_e
    mov     r11, rax
    mov     rax, r13
    call    overlap_boxes_at
    test    eax, eax
    jz      .pb_next_e
    mov     eax, HIT_PLAYER_BULLET_ENEMY
    push    rbx
    mov     ebx, r12d
    mov     ecx, r14d
    RECORD_HIT
    pop     rbx
    mov     rdi, rbx
    mov     esi, r14d
    call    score_add_enemy_kill
    mov     edx, r12d
    mov     esi, ENTITY_KIND_BULLET
    mov     rdi, rbx
    call    entity_kill
    mov     edx, r14d
    mov     esi, ENTITY_KIND_ENEMY
    mov     rdi, rbx
    call    entity_kill
.pb_next_e:
    inc     r14d
    jmp     .pb_enemy
.pb_next_b:
    inc     r12d
    jmp     .pb_bullet

    ; --- enemy bullets vs player ---
.enemy_bullets:
    lea     r13, [rbx + GS_PLAYER]
    xor     r12d, r12d
.eb_bullet:
    cmp     r12d, MAX_BULLETS
    jge     .body_hits
    mov     eax, r12d
    imul    eax, BULLET_SIZE
    lea     rax, [rbx + rax + GS_BULLETS]
    cmp     byte [rax + B_ACTIVE], 0
    je      .eb_next
    cmp     byte [rax + B_FROM_PLAYER], 0
    je      .eb_check
    jmp     .eb_next
.eb_check:
    mov     r11, r13
    ; rax = bullet pointer
    call    overlap_boxes_at
    test    eax, eax
    jz      .eb_next
    mov     eax, HIT_ENEMY_BULLET_PLAYER
    push    rbx
    mov     ebx, r12d
    mov     ecx, -1
    RECORD_HIT
    pop     rbx
    mov     edx, r12d
    mov     esi, ENTITY_KIND_BULLET
    mov     rdi, rbx
    call    entity_kill
    mov     rdi, rbx
    call    player_lose_life
.eb_next:
    inc     r12d
    jmp     .eb_bullet

    ; --- enemy body vs player ---
.body_hits:
    lea     r13, [rbx + GS_PLAYER]
    xor     r14d, r14d
.body_loop:
    cmp     r14d, MAX_ENEMIES
    jge     .done
    mov     eax, r14d
    imul    eax, ENEMY_SIZE
    lea     rax, [rbx + rax + GS_ENEMIES]
    cmp     byte [rax + E_ACTIVE], 0
    je      .body_next
    mov     r11, r13
    ; rax = enemy pointer
    call    overlap_boxes_at
    test    eax, eax
    jz      .body_next
    mov     eax, HIT_ENEMY_BODY_PLAYER
    push    rbx
    mov     ebx, -1
    mov     ecx, r14d
    RECORD_HIT
    pop     rbx
    mov     edx, r14d
    mov     esi, ENTITY_KIND_ENEMY
    mov     rdi, rbx
    call    entity_kill
    mov     rdi, rbx
    call    player_lose_life
.body_next:
    inc     r14d
    jmp     .body_loop

.done:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
