#include "gl.hpp"
#include "util/util.hpp"

namespace gl {

int poll_errors_and_warn ()
{
	int n = 0;
	for (GLenum err; (err = glGetError()) != GL_NO_ERROR; n++)
		WARNING("OpenGL error: {0} (0x{0:04X})", err);
	if (n > 0)
		WARNING("{:=>30}", ' ');
	return n;
}

void bind_ubo (UBO_binding_point slot, GLuint id)
{
	glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<GLenum>(slot), id);
}

void bind_ssbo (SSBO_binding_point slot, GLuint id)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<GLenum>(slot), id);
}


} // namespace gl
