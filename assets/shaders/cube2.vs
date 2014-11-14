#version 330 core

uniform mat4 m, v, p;
uniform mat3 normalMatrix;

in vec3 v_coord;
in vec3 v_normal;
in vec2 v_uv;

out vec4 f_coord;
out vec3 f_normalDirection;
out vec2 f_uv;


void main(void) {
	f_coord = vec4(v_coord, 1);
	f_normalDirection = normalize(normalMatrix * v_normal);
	f_uv = v_uv;

	mat4 mvp = p * v * m;
	gl_Position = mvp * f_coord;
}
