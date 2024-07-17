out vec4 final_color;

noperspective in vec2 id_factor;

//const vec3 top_left = vec3(26, 232, 180) / 255;
//const vec3 btm_left = vec3(6, 75, 103) / 255;

const vec3 top_rght = vec3(219, 143, 37) / 255;
const vec3 btm_rght = vec3(135, 16, 131) / 255;

const vec3 btm_left = top_rght;
const vec3 top_left = btm_rght;

void main ()
{
	const vec3 top = mix(top_left, top_rght, id_factor.x);
	const vec3 btm = mix(btm_left, btm_rght, id_factor.x);
	const vec3 color = mix(btm, top, id_factor.y);

	final_color = vec4(color, 1);
}
