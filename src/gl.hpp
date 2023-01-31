#ifndef GL_HPP
#define GL_HPP

#include "util/unique_handle.hpp"
#include "util/util.hpp"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <string_view>

/*
 * Some OpenGL wrappers, where they make sense.
 * Interoperability with raw OpenGL calls is a goal and intended usage.
 * Exhaustive wrapping is a non-goal.
 */
namespace gl {

void poll_errors_and_warn (std::string_view tag);
void poll_errors_and_die (std::string_view tag);

/* ========================== Basic OpenGL handle wrappers ========================== */

namespace detail {
struct Buffer_deleter { void operator() (GLuint id) { glDeleteBuffers(1, &id); } };
struct Texture_deleter { void operator() (GLuint id) { glDeleteTextures(1, &id); } };
struct FBO_deleter { void operator() (GLuint id) { glDeleteFramebuffers(1, &id); } };
struct VAO_deleter { void operator() (GLuint id) { glDeleteVertexArrays(1, &id); } };
}

using Buffer = Unique_handle<GLuint, detail::Buffer_deleter, 0>;
using Texture = Unique_handle<GLuint, detail::Texture_deleter, 0>;
using Framebuffer = Unique_handle<GLuint, detail::FBO_deleter, 0>;
using Vertex_array = Unique_handle<GLuint, detail::VAO_deleter, 0>;

inline Buffer make_buffer () { GLuint id; glCreateBuffers(1, &id); return Buffer(id); }
inline Texture make_texture () { GLuint id; glGenTextures(1, &id); return Texture(id); }
inline Framebuffer make_framebuffer () { GLuint id; glGenFramebuffers(1, &id); return Framebuffer(id); }
inline Vertex_array make_vertex_array () { GLuint id; glGenVertexArrays(1, &id); return Vertex_array(id); }

// ================================= Mapping buffers ================================= */

template <typename T> T* map_buffer_as (const Buffer& buffer, GLenum access)
{
	return start_lifetime_as<T>(glMapNamedBuffer(buffer.get(), access));
}

template <typename T> T* map_buffer_range_as
(const Buffer& buffer, size_t offs_bytes, size_t len_bytes, GLbitfield access)
{
	return start_lifetime_as<T>
		(glMapNamedBufferRange(buffer.get(), offs_bytes, len_bytes, access));
}

inline void flush_mapped_buffer_range
(const Buffer& buffer, size_t offs_bytes, size_t len_bytes)
{
	glFlushMappedNamedBufferRange(buffer.get(), offs_bytes, len_bytes);
}

inline void unmap_buffer (const Buffer& buffer)
{
	glUnmapNamedBuffer(buffer.get());
}

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

inline void bind_ubo (UBO_binding_point slot, const Buffer& buffer)
{
	glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<GLenum>(slot), buffer.get());
}

inline void bind_ssbo (SSBO_binding_point slot, const Buffer& buffer)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<GLenum>(slot), buffer.get());
}

} /* namespace gl */

#endif /* GL_HPP */
