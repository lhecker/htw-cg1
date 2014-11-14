static const char cube_vs[] = R"(
#version 330 core

uniform mat4 mvp;
in vec4 coord;
out vec4 texcoord;

void main(void) {
	texcoord = coord;
	gl_Position = mvp * vec4(coord.xyz, 1);
}
)";


static const char cube_fs[] = R"(
#version 330 core

uniform sampler2D tex;
in vec4 texcoord;
out vec4 fColor;

const vec4 fogcolor = vec4(0.6, 0.8, 1.0, 1.0);
const float fogdensity = 0.0001;

void main(void) {
	vec2 coord2d = vec2((fract(texcoord.x + texcoord.z) + texcoord.w) / 16.0, 1.0 - texcoord.y);
	vec4 color = texture(tex, coord2d);

	if(color.a < 0.4) {
		discard;
	}

	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fog = clamp(exp(-fogdensity * z * z), 0.0, 1.0);

	fColor = mix(fogcolor, color, fog);
}
)";

static const char red_vs[] = R"(
#version 330 core

in vec4 vPosition;

void main() {
	gl_Position = vPosition;
}
)";

static const char red_fs[] = R"(
#version 330 core

out vec4 fColor;

void main() {
	fColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)";
