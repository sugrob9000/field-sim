#ifndef GFX_HPP
#define GFX_HPP

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>

namespace gfx {
void init (unsigned res_x, unsigned res_y);
void deinit ();

void handle_sdl_event (const SDL_Event&);

void fieldviz_draw (bool should_clear);
void fieldviz_update ();

void present_frame ();
}

#endif /* GFX_HPP */
