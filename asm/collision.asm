;------------------------------------------------------------------------------
; collision.asm
;
; Galaga-assembly — axis-aligned bounding box overlap test (x86-64, NASM)
;
; System V AMD64 ABI (Linux, macOS): rdi, rsi, rdx, rcx, r8, r9
; Windows x64 ABI: rcx, rdx, r8, r9 (handled via separate macro if needed)
;
; Rect layout (must match include/galaga.h):
;   +0  int x
;   +4  int y
;   +8  int w
;   +12 int h
;
; History:
;   2026-07-01  Initial implementation
;------------------------------------------------------------------------------

%ifidn __OUTPUT_FORMAT__,win64
    %define RECT_X 0
    %define RECT_Y 4
    %define RECT_W 8
    %define RECT_H 12
    %xdefine A_PTR rcx
    %xdefine B_PTR rdx
%else
    %define RECT_X 0
    %define RECT_Y 4
    %define RECT_W 8
    %define RECT_H 12
    %xdefine A_PTR rdi
    %xdefine B_PTR rsi
%endif

section .text

; int rect_overlap(const Rect *a, const Rect *b)
; Returns 1 if overlapping, 0 if not.
global rect_overlap
rect_overlap:
    ; Load a: (x, y, w, h)
    mov  eax, [A_PTR + RECT_X]
    mov  ecx, [A_PTR + RECT_Y]
    mov  edx, [A_PTR + RECT_W]
    mov  r8d, [A_PTR + RECT_H]

    ; b.x
    mov  r9d, [B_PTR + RECT_X]
    ; a.left >= b.right  =>  a.x >= b.x + b.w  =>  no overlap
    mov  r10d, [B_PTR + RECT_W]
    add  r10d, r9d
    cmp  eax, r10d
    jge  .no_overlap

    ; b.left >= a.right
    lea  r10d, [rax + rdx]
    cmp  r9d, r10d
    jge  .no_overlap

    ; Y axis: a.y
    ; b.y
    mov  eax, [B_PTR + RECT_Y]
    mov  edx, [B_PTR + RECT_H]
    ; a.top >= b.bottom
    lea  r10d, [rax + rdx]
    cmp  ecx, r10d
    jge  .no_overlap

    ; b.top >= a.bottom
    mov  edx, [A_PTR + RECT_H]
    lea  r10d, [rcx + rdx]
    cmp  eax, r10d
    jge  .no_overlap

    mov  eax, 1
    ret

.no_overlap:
    xor  eax, eax
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
