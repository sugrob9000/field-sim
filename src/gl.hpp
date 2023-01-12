#ifndef GL_HPP
#define GL_HPP

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "util/unique_handle.hpp"

/* Some helpful OpenGL wrappers */
namespace gl {

void poll_errors_and_warn (std::string_view tag);
void poll_errors_and_die (std::string_view tag);

/* ========================== Basic OpenGL handle wrappers ========================== */

namespace detail {
struct Texture_deleter { void operator() (GLuint id) { glDeleteTextures(1, &id); } };
struct Framebuffer_deleter { void operator() (GLuint id) { glDeleteFramebuffers(1, &id); } };
}

using Texture = Unique_handle<GLuint, detail::Texture_deleter, 0>;
using Framebuffer = Unique_handle<GLuint, detail::Framebuffer_deleter, 0>;

inline GLuint new_framebuffer () { GLuint id; glGenFramebuffers(1, &id); return id; }
inline GLuint new_texture () { GLuint id; glGenTextures(1, &id); return id; }


/* ========================== Shader buffer bindings ========================== */
/*
 * This file needs to know about all binding point uses, which makes some sense because
 *  a) they are a global resource
 *  b) shaders need to specify them as numbers which need to match the numbers here
 *     anyway, and there is no preprocessing for shaders in place to enforce this
 *
 * However, doing this limits the total amount of available bindings by the number
 * of simultaneous binding points, akin to only ever using one descriptor set in Vulkan
 * TODO: fix the above, maybe by replicating the descriptor set design
 */

enum class UBO_binding_point: GLenum {
	// All uniform buffer binding points known in the program
	fieldviz_actors = 0,
};

enum class SSBO_binding_point: GLenum {
	// All shader storage buffer binding points known in the program
	fieldviz_particles = 0,
};

inline void bind_ubo (UBO_binding_point slot, GLuint id)
{
	glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<GLenum>(slot), id);
}

inline void bind_ssbo (SSBO_binding_point slot, GLuint id)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<GLenum>(slot), id);
}

} /* namespace gl */

#endif /* GL_HPP */
