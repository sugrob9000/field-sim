#include "glsl.hpp"
#include "util/util.hpp"
#include <algorithm>
#include <fstream>
#include <memory>
#include <optional>

using std::string_view;

namespace gl {
// ============================= Shader sources from files =============================
// A dumb implementation that supports #include, but leaves other preprocessing
// directives to the driver, which means you cannot guard inclusion with #if and friends:
// - you cannot prevent the inclusion of a file from happening;
//   if there is an #include in the source, it will be attempted
// - you cannot prevent recursive inclusion (in general, there is a limited depth)

class File_source {
	constexpr static const char source_dir[] = "shader/";
	constexpr static bool line_directive_has_filename = true;

	std::string src;
	std::string original_path;

	static std::optional<string_view> try_get_include_filename (string_view line) {
		using std::find, std::find_if, std::find_if_not;
		constexpr int (*is_space) (int) = std::isspace; // resolve overload
		constexpr string_view keyword = "include";

		auto end = line.end();
		auto hash = find_if_not(line.begin(), end, is_space);
		if (hash == end || *hash != '#')
			return {};

		auto directive_begin = find_if_not(hash+1, end, is_space);

		// '#include x' <- there must be at least two characters after
		// the keyword, the first of which also has to be whitespace

		if (static_cast<size_t>(end-directive_begin) < keyword.size()+2)
			return {};

		auto directive_end = directive_begin + keyword.size();
		if (!is_space(*directive_end)
		|| string_view{directive_begin, directive_end} != keyword)
			return {};

		auto file_begin = find_if_not(directive_end+1, end, is_space);
		if (file_begin == end)
			return {};

		if (*file_begin == '\"')
			return string_view{ file_begin+1, find(file_begin+1, end, '\"') };
		else
			return string_view{ file_begin, find_if(file_begin+1, end, is_space) };
	}

	void append_line_directive (unsigned long line_nr, [[maybe_unused]] string_view name) {
		auto out = std::back_inserter(src);
		if constexpr (line_directive_has_filename)
			fmt::format_to(out, FMT_STRING("#line {} \"{}{}\"\n"), line_nr, source_dir, name);
		else
			fmt::format_to(out, FMT_STRING("#line {}\n"), line_nr);
	}

	void append_from_file (string_view path, int recurse) {
		if (constexpr int limit = 20; recurse > limit) {
			FATAL("Shader '{}{}' has a recursive #include chain of depth > {}",
					source_dir, original_path, limit);
		}

		std::ifstream stream(std::string{source_dir} + std::string{path});
		if (!stream.good()) {
			if (recurse == 0) {
				FATAL("Shader '{}{}': cannot open file", source_dir, path);
			} else {
				FATAL("Shader '{0}{1}' (included from '{0}{2}'): cannot open file",
						source_dir, path, original_path);
			}
		}

		append_line_directive(0, path);

		unsigned long line_nr = 1;
		std::string line;
		for (; std::getline(stream, line); line_nr++) {
			if (auto include_target = try_get_include_filename(line)) {
				append_line_directive(0, *include_target);
				append_from_file(*include_target, recurse+1);
				append_line_directive(line_nr+1, path);
			} else {
				src += line;
				src += '\n';
			}
		}
	}

public:
	explicit File_source (string_view path)
		: original_path{path} { append_from_file(path, 0); }
	operator std::string () && { return std::move(src); }
};

// ================================== Loading shaders ==================================

constexpr static const char shader_prologue[] =
"#version 430 core\n"
"#extension GL_ARB_explicit_uniform_location: require\n"
"#extension GL_ARB_shading_language_include: require\n";

// `src` is std::string because we need zero-termination for glShaderSource,
// so would have made a std::string anyway
static Shader compile_shader (Shader_type type, const std::string& src, string_view name)
{
	GLuint id = glCreateShader(static_cast<GLenum>(type));
	if (id == 0)
		FATAL("Shader {}: failed to create shader object", name);

	const char* lines[] = { shader_prologue, src.c_str() };
	glShaderSource(id, std::size(lines), lines, nullptr);
	glCompileShader(id);

	int compile_success = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &compile_success);
	if (!compile_success) {
		int log_length = 0;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
		auto log = std::make_unique_for_overwrite<char[]>(log_length+1);
		glGetShaderInfoLog(id, log_length, &log_length, log.get());
		FATAL("Shader {} failed to compile. Log:\n{}", name, log.get());
	}

	return Shader(id);
}

Shader Shader::from_file (Shader_type type, string_view file_path)
{
	return compile_shader(type, File_source{file_path}, file_path);
}

Shader Shader::from_source (Shader_type type, string_view source)
{
	return compile_shader(type, std::string{source}, "<source string>");
}

// ================================== Shader programs ==================================

static GLuint link_program_low (std::span<const Shader> shaders)
{
	if (shaders.empty())
		FATAL("Tried to link a program without any shaders");

	GLuint id = glCreateProgram();
	if (id == 0)
		FATAL("Failed to create shader program");

	for (const Shader& s: shaders) glAttachShader(id, s.get());
	glLinkProgram(id);
	for (const Shader& s: shaders) glDetachShader(id, s.get());

	int link_success = 0;
	glGetProgramiv(id, GL_LINK_STATUS, &link_success);
	if (!link_success) {
		int log_length = 0;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
		auto log = std::make_unique_for_overwrite<char[]>(log_length+1);
		glGetProgramInfoLog(id, log_length, &log_length, log.get());
		FATAL("Program with id {} failed to link. Log:\n{}", id, log.get());
	}

	return id;
}

Program::Program (std::span<const Shader> shaders)
	: Unique_handle(link_program_low(shaders)) { }

Program Program::from_frag_vert (string_view frag_path, string_view vert_path)
{
	Shader shaders[] = {
		Shader::from_file(Shader_type::fragment, frag_path),
		Shader::from_file(Shader_type::vertex, vert_path),
	};
	return Program(shaders);
}

Program Program::from_compute (string_view compute_path)
{
	Shader shader[] = { Shader::from_file(Shader_type::compute, compute_path) };
	return Program(shader);
}

std::string Program::get_printable_internals () const
{
	int expected_length = 0;
	glGetProgramiv(this->get(), GL_PROGRAM_BINARY_LENGTH, &expected_length);

	const auto text = std::make_unique_for_overwrite<char[]>(expected_length);

	GLsizei real_length;
	GLenum bin_format;
	glGetProgramBinary(this->get(), expected_length, &real_length, &bin_format, text.get());

	// Filter out unprintable characters and hope the result is useful, godspeed
	std::string result;
	for (char c: std::string_view{ text.get(), text.get()+real_length }) {
		if (c == '\t' || c == '\n' || (c >= 0x20 && c <= 0x7E))
			result.push_back(c);
	}
	return result;
}

} // namespace gl
