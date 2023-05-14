#pragma once

#include "util/unique.hpp"
#include "util/util.hpp"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <string_view>

// Some OpenGL wrappers, for better C++ compatibility.
// Complete interoperability with raw OpenGL calls is intended. Exhaustive wrapping is not.
namespace gl {

void poll_errors_and_warn (std::string_view tag);
void poll_errors_and_die (std::string_view tag);

inline std::string_view get_string (GLenum id) { return reinterpret_cast<const char*>(glGetString(id)); }

// ========================== Basic OpenGL handle wrappers ==========================
// TODO: Gen* calls don't actually create valid OpenGL objects until the names returned
// are bound to a target, meaning the Unique_handle objects returned by gl::gen* functions
// may not represent actual objects even with a nontrivial ID.
// This is the reason that e.g. gl::gen_texture does not take the texture target or any
// other parameters, though it logically should.
// glDelete* functions are specified to swallow invalid names silently, but it'd be
// cleaner to get rid of gen* and make create* for all of this API; in some places it will
// require eschewing non-DSA usage completely.

namespace detail {
// If the deleter is imported directly, `GL_delete_func` is `void(*)(GLsizei, GLuint*)`.
// If it is imported by the loader via GetProcAddress, the name of the function
// resolves to a variable, and `GL_delete_func` is `void(**)(GLsizei, GLuint*)`.
// So, `GL_delete_func` has to be `auto`, and calling `*GL_delete_func` works either way.
template <auto GL_delete_func> struct GL_obj_deleter {
	void operator() (GLuint id) const noexcept { (*GL_delete_func)(1, &id); }
};
} // namespace detail
using Buffer = Unique_handle<GLuint, detail::GL_obj_deleter<&glDeleteBuffers>, 0>;
using Texture = Unique_handle<GLuint, detail::GL_obj_deleter<&glDeleteTextures>, 0>;
using Renderbuffer = Unique_handle<GLuint, detail::GL_obj_deleter<&glDeleteRenderbuffers>, 0>;
using Framebuffer = Unique_handle<GLuint, detail::GL_obj_deleter<&glDeleteFramebuffers>, 0>;
using Vertex_array = Unique_handle<GLuint, detail::GL_obj_deleter<&glDeleteVertexArrays>, 0>;

inline auto gen_buffer () { GLuint id; glGenBuffers(1, &id); return Buffer(id); }
inline auto create_buffer () { GLuint id; glCreateBuffers(1, &id); return Buffer(id); }

inline auto gen_renderbuffer () { GLuint id; glGenRenderbuffers(1, &id); return Renderbuffer(id); }
inline auto create_renderbuffer () { GLuint id; glCreateRenderbuffers(1, &id); return Renderbuffer(id); }

inline auto gen_texture () { GLuint id; glGenTextures(1, &id); return Texture(id); }
inline auto create_texture (GLenum target)
{
	GLuint id;
	glCreateTextures(target, 1, &id);
	return Texture(id);
}

inline auto gen_framebuffer () { GLuint id; glGenFramebuffers(1, &id); return Framebuffer(id); }
inline auto gen_vertex_array () { GLuint id; glGenVertexArrays(1, &id); return Vertex_array(id); }

// ================================= Mapping buffers =================================

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


// ========================== Shader buffer bindings ========================== */
// This file needs to know about all binding point uses, which makes some sense because
//  a) they are a global resource
//  b) shaders need to specify them as numbers which need to match the numbers here
//     anyway, and there is no preprocessing for shaders in place to enforce this
//
// However, doing this limits the total amount of available bindings by the number
// of simultaneous binding points, akin to only ever using one descriptor set in Vulkan
// TODO: fix the above, maybe by replicating the descriptor set design

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

} // namespace gl
