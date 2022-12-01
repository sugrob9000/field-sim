#ifndef GL_HPP
#define GL_HPP

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "util/unique_handle.hpp"

namespace gl {

struct Texture_deleter_ { void operator() (GLuint id) { glDeleteTextures(1, &id); } };
using Texture = Unique_handle<GLuint, Texture_deleter_, 0>;

struct Framebuffer_deleter_ { void operator() (GLuint id) { glDeleteFramebuffers(1, &id); } };
using Framebuffer = Unique_handle<GLuint, Framebuffer_deleter_, 0>;

inline GLuint gen_framebuffer () { GLuint id; glGenFramebuffers(1, &id); return id; }
inline GLuint gen_texture () { GLuint id; glGenTextures(1, &id); return id; }

int poll_errors_and_warn ();

} // namespace

#endif /* GL_HPP */
