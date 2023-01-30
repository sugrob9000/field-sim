#ifndef MATH_HPP
#define MATH_HPP

#include "util/util.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

using glm::vec2, glm::vec3, glm::vec4;
using glm::mat2, glm::mat3, glm::mat4;

/*
 * Format a GLM vector like "(0.0, 1.0, 2.0)"
 * reusing format specifiers of the underlying type, for example:
 *
 *   fmt::format("{:#x}", glm::vec<3,int>(100,200,300)) -> "(0x64, 0xc8, 0x12c)"
 */
template <int N, typename S, glm::qualifier Q>
class fmt::formatter<glm::vec<N,S,Q>>: public formatter<S> {
	constexpr static auto opening = FMT_STRING("(");
	constexpr static auto joiner = FMT_STRING(", ");
	constexpr static auto closing = FMT_STRING(")");
public:
	template <typename Context> auto format (const glm::vec<N,S,Q>& v, Context& ctx) const {
		format_to(ctx.out(), opening);
		for (int i = 0; i+1 < N; i++) {
			formatter<S>::format(v[i], ctx);
			format_to(ctx.out(), joiner);
		}
		formatter<S>::format(v[N-1], ctx);
		return format_to(ctx.out(), closing);
	}
};

#endif /* MATH_HPP */
