#pragma once

#include "util/unique.hpp"
#include <GL/glew.h>
#include <span>
#include <string>
#include <string_view>

namespace gl {

enum class Shader_type: GLenum {
	fragment = GL_FRAGMENT_SHADER,
	vertex = GL_VERTEX_SHADER,
	geometry = GL_GEOMETRY_SHADER,
	compute = GL_COMPUTE_SHADER,
	tess_control = GL_TESS_CONTROL_SHADER,
	tess_eval = GL_TESS_EVALUATION_SHADER,
};

namespace detail {
struct Shader_deleter { void operator() (GLuint id) { glDeleteShader(id); } };
struct Program_deleter { void operator() (GLuint id) { glDeleteProgram(id); } };
}

struct Shader: Unique_handle<GLuint, detail::Shader_deleter, 0> {
	using Unique_handle::Unique_handle;
	static Shader from_file (Shader_type, std::string_view file_path);
	static Shader from_source (Shader_type, std::string_view source);
};

struct Program: Unique_handle<GLuint, detail::Program_deleter, 0> {
	using Unique_handle::Unique_handle;

	// General case: an arbitrary collection of shader objects
	explicit Program (std::span<const Shader>);

	// Shorthands for the two common cases
	static Program from_frag_vert (std::string_view frag_path, std::string_view vert_path);
	static Program from_compute (std::string_view comp_path);

	// Get a non-portable string of printable characters in the output of glGetProgramBinary.
	// Nvidia drivers at least include a high-level assembly listing in there
	[[nodiscard]] std::string get_printable_internals () const;
};

} // namespace gl
