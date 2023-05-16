#pragma once

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <fmt/format.h>

using glm::vec2, glm::vec3, glm::vec4;
using glm::mat2, glm::mat3, glm::mat4;

// Format a GLM vector like "(0.0, 1.0, 2.0)"
// reusing format specifiers of the underlying type, for example:
//   fmt::format("{:#x}", glm::vec<3,int>(100,200,300)) -> "(0x64, 0xc8, 0x12c)"
template <int N, typename S, glm::qualifier Q>
class fmt::formatter<glm::vec<N,S,Q>>: formatter<S> {
	constexpr static auto opening = FMT_STRING("(");
	constexpr static auto joiner = FMT_STRING(", ");
	constexpr static auto closing = FMT_STRING(")");
	using Base = formatter<S>;
public:
	using Base::parse;
	template <typename Context> auto format (const glm::vec<N,S,Q>& v, Context& ctx) const {
		format_to(ctx.out(), opening);
		for (int i = 0; i+1 < N; i++) {
			Base::format(v[i], ctx);
			format_to(ctx.out(), joiner);
		}
		Base::format(v[N-1], ctx);
		return format_to(ctx.out(), closing);
	}
};

// Foreach support for glm vectors
namespace glm {
template <int N, typename S, qualifier Q> S* begin (vec<N,S,Q>& v) { return value_ptr(v); }
template <int N, typename S, qualifier Q> S* end (vec<N,S,Q>& v) { return value_ptr(v)+N; }
template <int N, typename S, qualifier Q> const S* begin (const vec<N,S,Q>& v) { return value_ptr(v); }
template <int N, typename S, qualifier Q> const S* end (const vec<N,S,Q>& v) { return value_ptr(v)+N; }
}
