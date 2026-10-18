/* Renders the 48x48 source art for the PWA icons from the game sprites.
   Build: gcc -Iinclude $(sdl2-config --cflags) tools/icon_gen.c src/sprites.c $(sdl2-config --libs)
   Run with SDL_VIDEODRIVER=dummy; then tools/icon_gen.py scales it into web/icons/. */
#include "galaga.h"
#include <SDL.h>
int main(void){ SDL_Init(SDL_INIT_VIDEO); SDL_Window*w=SDL_CreateWindow("i",0,0,48,48,0); SDL_Renderer*r=SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE);
  sprites_init(r); SDL_SetRenderDrawColor(r,10,10,24,255); SDL_RenderClear(r);
  int st[][5]={{5,6,255,255,255},{40,9,255,90,90},{9,30,90,200,255},{42,34,255,230,80},{30,4,120,255,140},{20,40,160,160,255}};
  for(int i=0;i<6;i++){SDL_SetRenderDrawColor(r,st[i][2],st[i][3],st[i][4],255); SDL_RenderDrawPoint(r,st[i][0],st[i][1]);}
  SDL_Rect be={16,6,16,16}; SDL_RenderCopy(r,sprites_enemy(ENEMY_TYPE_BOSS,0,2),NULL,&be);
  SDL_SetRenderDrawColor(r,255,255,255,255); SDL_Rect bl={23,23,2,4}; SDL_RenderFillRect(r,&bl);
  SDL_Rect pl={16,29,16,16}; SDL_RenderCopy(r,sprites_player(),NULL,&pl);
  SDL_Surface*sf=SDL_CreateRGBSurfaceWithFormat(0,48,48,32,SDL_PIXELFORMAT_ARGB8888); SDL_RenderReadPixels(r,NULL,SDL_PIXELFORMAT_ARGB8888,sf->pixels,sf->pitch); SDL_SaveBMP(sf,"/tmp/icon48.bmp"); SDL_Quit(); return 0; }
