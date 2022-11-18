#ifndef MATH_HPP
#define MATH_HPP

#include "util/util.hpp"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

using glm::vec2, glm::vec3, glm::vec4;
using glm::mat2, glm::mat3, glm::mat4;

/*
 * Format a GLM vector with fmt in the format "(x, ...)",
 * reusing format specifiers of the underlying type, for example
 *
 *   fmt::format("{:#x}", glm::vec<3,int>(100,200,300)) -> "(0x64, 0xc8, 0x12c)"
 */
template <int N, typename S, glm::qualifier Q>
struct fmt::formatter<glm::vec<N,S,Q>>: formatter<S> {
	template <typename Context> auto format (const glm::vec<N,S,Q>& v, Context& ctx) const {
		const auto put = [&ctx] (char c) { return *ctx.out()++ = c; };
		put('(');
		for (int i = 0; i < N-1; i++) {
			this->formatter<S>::format(v[i], ctx);
			put(',');
			put(' ');
		}
		this->formatter<S>::format(v[N-1], ctx);
		return put(')');
	}
};

/*
 * Allow making structured bindings on GLM vectors, for example
 *   auto [x, y] = decompose(my_vec2)
 */
template <int N, typename S, glm::qualifier Q> auto decompose (const glm::vec<N,S,Q>& v)
{
	if constexpr (N == 2) return std::tie(v.x, v.y);
	if constexpr (N == 3) return std::tie(v.x, v.y, v.z);
	if constexpr (N == 4) return std::tie(v.x, v.y, v.z, v.w);
}

#endif /* MATH_HPP */