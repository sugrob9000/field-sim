#include "gl.hpp"

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

} // namespace