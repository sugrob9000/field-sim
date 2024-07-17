#pragma once

#include "util/unique.hpp"
#include "util/util.hpp"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <string_view>

// Some OpenGL wrappers, for better C++ compatibility.
// Complete interoperability with raw OpenGL calls is intended. Exhaustive wrapping is not.
namespace gl {

void poll_errors_and_warn(std::string_view tag);
void poll_errors_and_die(std::string_view tag);

void debug_message_callback(
  GLenum src,
  GLenum type,
  GLuint id,
  GLenum severe,
  GLsizei len,
  const char* msg,
  const void* param
);

inline std::string_view get_string(GLenum name) {
  return reinterpret_cast<const char*>(glGetString(name));
}

inline std::string_view get_string(GLenum name, GLuint index) {
  return reinterpret_cast<const char*>(glGetStringi(name, index));
}

// ============================== OpenGL handle wrappers ==============================
namespace detail {
// Creation and deletion functions have their address taken, so:
// if the deleter is imported directly, `GL_xxx_func` is `void(*)(...)`.
// if it is imported by the loader via GetProcAddress, the name of the function
// resolves to a variable, and the type is is `void(**)(...)`.
// So, `GL_xxx_func` have to be `auto`, and dereferencing before calling works either way

template<auto GL_delete_func>
struct GL_obj_deleter {
  void operator()(GLuint id) const noexcept {
    (*GL_delete_func)(1, &id);
  }
};

template<auto GL_create_func, auto GL_delete_func>
struct GL_basic_object: Unique_handle<GLuint, GL_obj_deleter<GL_delete_func>, 0> {
  using Unique_handle<GLuint, GL_obj_deleter<GL_delete_func>, 0>::Unique_handle;

  template<typename... Args>
  static GL_basic_object create(Args... args)
    requires requires(GLuint id) { (*GL_create_func)(args..., 1, &id); }
  {
    GLuint id;
    (*GL_create_func)(args..., 1, &id);
    return GL_basic_object(id);
  }
};
}  // namespace detail

using Buffer = detail::GL_basic_object<&glCreateBuffers, &glDeleteBuffers>;
using Renderbuffer = detail::GL_basic_object<&glCreateRenderbuffers, &glDeleteRenderbuffers>;
using Framebuffer = detail::GL_basic_object<&glCreateFramebuffers, &glDeleteFramebuffers>;
using Vertex_array = detail::GL_basic_object<&glCreateVertexArrays, &glDeleteVertexArrays>;
using Query = detail::GL_basic_object<&glCreateQueries, &glDeleteQueries>;
using Texture = detail::GL_basic_object<&glCreateTextures, &glDeleteTextures>;

// ================================= Mapping buffers =================================

template<typename T>
T* map_buffer_as(const Buffer& buffer, GLenum access) {
  return start_lifetime_as<T>(glMapNamedBuffer(buffer.get(), access));
}

template<typename T>
T* map_buffer_range_as(const Buffer& buffer, size_t offs_bytes, size_t len_bytes, GLbitfield access) {
  return start_lifetime_as<T>(glMapNamedBufferRange(buffer.get(), offs_bytes, len_bytes, access));
}

inline void flush_mapped_buffer_range(const Buffer& buffer, size_t offs_bytes, size_t len_bytes) {
  glFlushMappedNamedBufferRange(buffer.get(), offs_bytes, len_bytes);
}

inline void unmap_buffer(const Buffer& buffer) {
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

enum class UBO_binding_point : GLenum {
  // All uniform buffer binding points known in the program
  fieldviz_actors = 0,
};

enum class SSBO_binding_point : GLenum {
  // All shader storage buffer binding points known in the program
  fieldviz_particles = 0,
};

inline void bind_ubo(UBO_binding_point slot, const Buffer& buffer) {
  glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<GLenum>(slot), buffer.get());
}

inline void bind_ssbo(SSBO_binding_point slot, const Buffer& buffer) {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<GLenum>(slot), buffer.get());
}

}  // namespace gl
