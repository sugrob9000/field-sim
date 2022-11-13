layout (location = 0) in vec2 position;

layout (location = 0) uniform uvec2 grid_size;
layout (location = 2) uniform vec2 scale;

noperspective out vec2 id_factor;  // used for coloring, depends on the initial position

const uint ID = gl_VertexID / 2;
const uint ID_X = ID % grid_size.x;
const uint ID_Y = ID / grid_size.x;

void main ()
{
	id_factor = smoothstep(vec2(0.15), vec2(0.85),
			vec2(float(ID_X), float(ID_Y)) / grid_size);
	vec2 pos_adjusted = (2 * position / grid_size) - vec2(1);
	gl_Position = vec4(scale * pos_adjusted, 0.0, 1.0);
}
