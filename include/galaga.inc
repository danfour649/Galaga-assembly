;------------------------------------------------------------------------------
; galaga.inc — struct layout constants (must match include/galaga.h)
;------------------------------------------------------------------------------

%define GALAGA_WIDTH      224
%define GALAGA_HEIGHT     288

%define MAX_ENEMIES       40
%define MAX_BULLETS       8

%define ENTITY_KIND_ENEMY 0
%define ENTITY_KIND_BULLET 1

%define GS_PLAYER         0
%define GS_ENEMIES        28
%define GS_BULLETS        1148

%define ENEMY_SIZE        28
%define BULLET_SIZE       24

%define E_ACTIVE          0
%define E_X               4
%define E_Y               8
%define E_W               12
%define E_H               16
%define E_TYPE            20
%define E_HP              24

%define B_ACTIVE          0
%define B_X               4
%define B_Y               8
%define B_W               12
%define B_H               16
%define B_FROM_PLAYER     20

%define P_X               0
%define P_W               8

%define ENEMY_W_DEFAULT   14
%define ENEMY_H_DEFAULT   14
%define BULLET_W_DEFAULT  3
%define BULLET_H_DEFAULT  8
%define MAX_PLAYER_BULLETS 2
