#include "gl.hpp"
#include "util/util.hpp"

namespace gl {

static std::string_view error_code_name (GLenum error)
{
	switch (error) {
	case GL_INVALID_ENUM: return "invalid enum";
	case GL_INVALID_VALUE: return "invalid value";
	case GL_INVALID_OPERATION: return "invalid operation";
	case GL_STACK_OVERFLOW: return "stack overflow";
	case GL_STACK_UNDERFLOW: return "stack underflow";
	case GL_OUT_OF_MEMORY: return "out of memory";
	default: return "unknown error code";
	}
}

static int poll_errors_warn_on_each ()
{
	int num_errors = 0;
	for (GLenum error; (error = glGetError()) != GL_NO_ERROR; num_errors++)
		WARNING("OpenGL error: {0} (0x{0:04x}) - {1}", error, error_code_name(error));
	return num_errors;
}

void poll_errors_and_warn (std::string_view tag)
{
	if (int num_errors = poll_errors_warn_on_each(); num_errors > 0)
		WARNING("====== {} OpenGL error(s) reported during '{}' (see above)", num_errors, tag);
}

void poll_errors_and_die (std::string_view tag)
{
	if (int num_errors = poll_errors_warn_on_each(); num_errors > 0)
		FATAL("{} OpenGL error(s) reported during '{}'", num_errors, tag);
}

} // namespace gl
