#ifndef GFX_HPP
#define GFX_HPP

#include <GL/glew.h>
#include <SDL.h>

namespace gfx {
void init (unsigned res_x, unsigned res_y);
void deinit ();

void handle_sdl_event (const SDL_Event&);
void present_frame ();

void fieldviz_update ();
void fieldviz_draw (bool should_clear);
}

#endif /* GFX_HPP */
