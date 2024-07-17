out vec4 final_color;

noperspective in vec2 id_factor;

const vec3 top_left = vec3(26, 232, 180) / 255;
const vec3 bottom_left = vec3(6, 75, 103) / 255;

const vec3 top_right = vec3(219, 143, 37) / 255;
const vec3 bottom_right = vec3(135, 16, 131) / 255;

//const vec3 bottom_left = top_right;
//const vec3 top_left = bottom_right;

void main ()
{
	const vec3 top = mix(top_left, top_right, id_factor.x);
	const vec3 btm = mix(bottom_left, bottom_right, id_factor.x);
	const vec3 color = mix(btm, top, id_factor.y);

	final_color = vec4(color, 1);
}
