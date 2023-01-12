#include "gl.hpp"
#include "util/util.hpp"

namespace gl {

static int poll_warn_on_each ()
{
	int num_errors = 0;
	for (GLenum error; (error = glGetError()) != GL_NO_ERROR; num_errors++)
		WARNING("OpenGL error: {0} (0x{0:04x})", error);
	return num_errors;
}

void poll_errors_and_warn (std::string_view tag)
{
	if (int num_errors = poll_warn_on_each(); num_errors > 0)
		WARNING("====== {} OpenGL error(s) reported during <{}>", num_errors, tag);
}

void poll_errors_and_die (std::string_view tag)
{
	if (int num_errors = poll_warn_on_each(); num_errors > 0)
		FATAL("{} OpenGL error(s) reported during <{}>", num_errors, tag);
}

} // namespace gl
