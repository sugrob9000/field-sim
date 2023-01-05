#include "glsl.hpp"
#include "util/util.hpp"
#include <fstream>
#include <memory>
#include <optional>

namespace glsl {
// =============================== Loading shaders: core ===============================

constexpr static const char shader_prologue[] =
	"#version 430 core\n"
	"#extension GL_ARB_explicit_uniform_location: require\n"
	"#extension GL_ARB_shading_language_include: require\n";

static GLuint shader_compile (Shader_type type,
                              std::string src, // needs 0-termination, so owning
                              std::string_view display_path)
{
	GLuint id = glCreateShader(static_cast<GLenum>(type));
	if (id == 0)
		FATAL("Shader {}: failed to allocate shader object", display_path);

	const char* lines[] = { shader_prologue, src.c_str() };
	glShaderSource(id, std::size(lines), lines, nullptr);
	glCompileShader(id);

	int compile_success = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &compile_success);
	if (!compile_success) {
		int log_length = 0;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
		std::string log(log_length, '\0');
		glGetShaderInfoLog(id, log_length, &log_length, log.data());
		FATAL("Shader {} failed to compile. Log:\n{}", display_path, log);
	}

	return id;
}

Shader shader_from_string (Shader_type type, std::string_view source)
{
	return Shader{shader_compile(type, std::string{source}, "<source string>")};
}

// ============================= Loading shader from file =============================
// A dumb implementation that supports #include, but leaves other preprocessing
// directives to the driver, which means you cannot guard inclusion with #if and friends:
// - you cannot prevent the inclusion of a file from happening;
//   if there is an #include in the source, it will be attempted
// - you cannot prevent recursive inclusion (in general, there is a limited depth)

static std::optional<std::string> try_get_include_filename (std::string_view line)
{
	constexpr char keyword[] = "#include";
	constexpr size_t keyword_len = sizeof(keyword)-1; // exclude trailing \0

	if (line.size() <= keyword_len+1
	|| line.rfind(keyword, 0) != 0
	|| !std::isspace(line[keyword_len]))
		return std::nullopt;

	size_t begin = keyword_len+1;
	// skip whitespace
	while (begin < line.size() && std::isspace(line[begin]))
		begin++;

	size_t end = begin + 1;
	if (line[begin] == '\"') {
		// find a matching quote (if there isn't one, use EOL, whatever)
		begin++;
		while (end < line.size() && line[end] != '\"')
			end++;
	} else {
		// find next whitespace or EOL
		while (end < line.size() && !std::isspace(line[end]))
			end++;
	}

	return std::string(line.data()+begin, line.data()+end);
}

static void append_line_directive (std::string& src, int line_nr,
                                   [[maybe_unused]] std::string_view name)
{
	src += fmt::format(FMT_STRING("#line {} \"{}\"\n"), line_nr, name);
}

constexpr static const char source_dir[] = "shader/";

static void append_file_contents (std::string& src,
                                  std::string_view original_path,
                                  std::string_view path,
                                  int recurse = 0)
{
	if (constexpr int limit = 20; recurse > limit) {
		FATAL("Shader '{}{}' has a recursive #include chain of depth > {}. "
		      "Note that #include is not guarded by conditional compilation directives",
		      source_dir, original_path, limit);
	}

	std::ifstream f(std::string{source_dir} + std::string{path});
	if (!f.good()) {
		if (recurse == 0) {
			FATAL("Shader '{}{}': cannot open file", source_dir, path);
		} else {
			FATAL("Shader '{0}{1}' (included from '{0}{2}'): cannot open file",
			      source_dir, path, original_path);
		}
	}

	append_line_directive(src, 0, path);

	int line_nr = 1;
	std::string line;
	for (; std::getline(f, line); line_nr++) {
		if (auto target = try_get_include_filename(line)) {
			append_line_directive(src, 0, *target);
			append_file_contents(src, original_path, *target, recurse+1);
			append_line_directive(src, line_nr+1, path);
		} else {
			src += line;
			src += '\n';
		}
	}
}

Shader shader_from_file (Shader_type type, std::string_view file_path)
{
	std::string source;
	append_file_contents(source, file_path, file_path);
	return Shader{shader_compile(type, source, file_path)};
}

// ============================== Loading programs: core ==============================

GLuint link_program (std::span<const Shader> shaders)
{
	if (shaders.empty())
		FATAL("Tried to link a program without any shaders");

	GLuint program_id = glCreateProgram();
	if (program_id == 0)
		FATAL("Failed to allocate shader program");

	for (const Shader& s: shaders)
		glAttachShader(program_id, s.get());
	glLinkProgram(program_id);
	for (const Shader& s: shaders)
		glDetachShader(program_id, s.get());

	int link_success = 0;
	glGetProgramiv(program_id, GL_LINK_STATUS, &link_success);
	if (!link_success) {
		int log_length = 0;
		glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
		std::string log(log_length+1, '\0');
		glGetProgramInfoLog(program_id, log_length, &log_length, log.data());
		FATAL("Program with id {} failed to link. Log:\n{}", program_id, log);
	}

	return program_id;
}

void delete_program (GLuint id)
{
	glDeleteProgram(id);
}

std::string get_printable_program_internals (GLuint program_id)
{
	int length = 0;
	glGetProgramiv(program_id, GL_PROGRAM_BINARY_LENGTH, &length);

	auto buffer = std::make_unique<char[]>(length);
	char* ptr = buffer.get();

	GLsizei real_length;
	GLenum bin_format;
	glGetProgramBinary(program_id, length, &real_length, &bin_format, ptr);

	// Filter out unprintable characters and hope the result is useful, godspeed
	std::string result;
	for (char c: std::string_view{ ptr, ptr+real_length }) {
		if (c == '\t' || c == '\n' || (c >= 0x20 && c <= 0x7E))
			result.push_back(c);
	}
	return result;
}

// ========================= Linking programs: small helpers =========================

GLuint make_program_frag_vert (std::string_view frag_path, std::string_view vert_path)
{
	Shader shaders[] = {
		shader_from_file(Shader_type::fragment, frag_path),
		shader_from_file(Shader_type::vertex, vert_path),
	};
	return link_program(shaders);
}

GLuint make_program_compute (std::string_view comp_path)
{
	Shader shader[] = { shader_from_file(Shader_type::compute, comp_path) };
	return link_program(shader);
}

} // namespace
