layout (location = 0) in vec2 position;

layout (location = 0) uniform uvec2 workgroup_size;
layout (location = 2) uniform uvec2 workgroup_num;

layout (location = 4) uniform vec2 scale;

noperspective out vec2 id_factor;

void main ()
{
	uint line_id = gl_VertexID / 2;

	vec2 grid_size = workgroup_num * workgroup_size;

	uint wg_size_total = workgroup_size.x * workgroup_size.y;
	uint wg_id = line_id / wg_size_total;
	uint loc_id = line_id % wg_size_total;

	vec2 wg = vec2(float(wg_id % workgroup_num.x), float(wg_id / workgroup_num.x));
	vec2 loc = vec2(float(loc_id % workgroup_size.x), float(loc_id / workgroup_size.y));
	vec2 coord = wg * workgroup_size + loc;

	id_factor = smoothstep(vec2(0.15), vec2(0.85), coord / grid_size);

	vec2 pos_adjusted = (2 * position / grid_size) - vec2(1);
	gl_Position = vec4(scale * pos_adjusted, 0.0, 1.0);
}
