#ifndef GLSL_HPP
#define GLSL_HPP

#include "util.hpp"
#include <GL/glew.h>
#include <span>
#include <string>
#include <string_view>

namespace glsl {
/*
 * Shorthands for the two common cases.
 *  - A single fragment shader source & a single vertex shader source
 *  - A single compute shader source
 * These functions compile and dispose of shader objects properly.
 */
GLuint make_program_frag_vert (std::string_view frag_path, std::string_view vert_path);
GLuint make_program_compute (std::string_view comp_path);

/*
 * Management of shader objects. RAII is more useful for shaders, because they can be
 * disposed of after being linked into a program, much like object files
 */
enum class Shader_type: GLenum {
	fragment = GL_FRAGMENT_SHADER,
	vertex = GL_VERTEX_SHADER,
	geometry = GL_GEOMETRY_SHADER,
	compute = GL_COMPUTE_SHADER,
	tess_control = GL_TESS_CONTROL_SHADER,
	tess_eval = GL_TESS_EVALUATION_SHADER,
};

struct Shader_deleter_ { void operator() (GLuint id) { glDeleteShader(id); } };
using Shader = Unique_handle<GLuint, Shader_deleter_, 0>;

Shader shader_from_file (Shader_type, std::string_view file_path);
Shader shader_from_string (Shader_type, std::string_view source);

/*
 * Management of shader programs. RAII is less useful because shader programs
 * are likely to live for as long as the rendering engine does, so it's manual here
 */
GLuint link_program (std::span<const Shader>);
void delete_program (GLuint);

/*
 * Get a non-portable string of printable characters in the output of glGetProgramBinary.
 * Nvidia drivers at least include a high-level assembly listing in there
 */
std::string get_printable_program_internals (GLuint);

} /* namespace */

#endif /* GLSL_HPP */
