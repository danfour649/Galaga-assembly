;------------------------------------------------------------------------------
; galaga.inc — struct layout constants (must match include/galaga.h)
;------------------------------------------------------------------------------

%define GALAGA_WIDTH      224
%define GALAGA_HEIGHT     288

%define MAX_ENEMIES       40
%define MAX_BULLETS       8

%define ENTITY_KIND_ENEMY 0
%define ENTITY_KIND_BULLET 1

%define ENEMY_TYPE_BEE        0
%define ENEMY_STATE_FORMATION 0
%define ENEMY_STATE_DIVING    1

%define GS_PLAYER         0
%define GS_ENEMIES        28
%define GS_BULLETS        1788
%define GS_RUNNING        1980
%define GS_GAME_OVER      1981
%define GS_FRAME          1984
%define GS_PHASE          1988
%define GS_CLEAR_TIMER    1992
%define GS_NEXT_EXTRA_LIFE 1996

%define GAME_PHASE_TITLE       0
%define GAME_PHASE_PLAYING     1
%define GAME_PHASE_STAGE_CLEAR 2
%define GAME_PHASE_GAME_OVER   3

%define HIT_PLAYER_BULLET_ENEMY 0
%define HIT_ENEMY_BULLET_PLAYER 1
%define HIT_ENEMY_BODY_PLAYER   2

%define H_OFF_COUNT       0
%define H_OFF_TYPE        4
%define H_OFF_BULLET      36
%define H_OFF_ENEMY       68
%define MAX_COLLISION_HITS 8

%define SCORE_BEE_FORMATION 50
%define SCORE_BEE_DIVING    100
%define EXTRA_LIFE_1        20000
%define EXTRA_LIFE_2        70000
%define EXTRA_LIFE_3        150000
%define STAGE_CLEAR_FRAMES  90

%define ENEMY_SIZE        44
%define BULLET_SIZE       24

%define E_ACTIVE          0
%define E_X               4
%define E_Y               8
%define E_W               12
%define E_H               16
%define E_TYPE            20
%define E_STATE           24
%define E_HOME_X          28
%define E_HOME_Y          32
%define E_VX              36
%define E_FIRE_CD         40

%define B_ACTIVE          0
%define B_X               4
%define B_Y               8
%define B_W               12
%define B_H               16
%define B_FROM_PLAYER     20

%define P_X               0
%define P_Y               4
%define P_W               8
%define P_H               12
%define P_LIVES           16
%define P_SCORE           20
%define P_STAGE           24

%define PLAYER_START_LIVES 3

%define ENEMY_W_DEFAULT   14
%define ENEMY_H_DEFAULT   14
%define BULLET_W_DEFAULT  3
%define BULLET_H_DEFAULT  8
%define MAX_PLAYER_BULLETS 2

; Formation layout (5 rows × 8 cols of bees)
%define FORM_COLS         8
%define FORM_ROWS         5
%define FORM_START_X      16
%define FORM_START_Y      28
%define FORM_GAP_X        24
%define FORM_GAP_Y        18

%define DIVE_INTERVAL     90
%define DIVE_FIRE_COOLDOWN 40
