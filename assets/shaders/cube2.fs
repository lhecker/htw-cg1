#version 330 core

struct lightSource {
	vec4 position;
	vec4 diffuse;
	vec3 spotDirection;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
	float spotCutoff;
	float spotExponent;
};

struct material {
	vec4 diffuse;
};


const lightSource light0 = lightSource(
	vec4(0.0, 14.0, 0.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec3(0.0, -1.0, 0.0),
	0.0,
	0.2,
	0.0,
	360.0,
	20.0
);

const material mymaterial = material(vec4(1.0, 0.8, 0.8, 1.0));

const vec4 fogcolor = vec4(0.6, 0.8, 1.0, 1.0);
const float fogdensity = 0.0001;


uniform mat4 m;
uniform sampler2D tex;

in vec4 f_coord;
in vec3 f_normalDirection;
in vec2 f_uv;

out vec4 f_color;


void main(void) {
	vec4 color = texture(tex, f_uv);

	if(color.a < 0.1) {
		discard;
	}

	vec3 lightDirection;
	float attenuation;

	if (light0.position.w == 0.0) {
		// directional light

		attenuation = 1.0; // no attenuation
		lightDirection = normalize(vec3(light0.position));
	} else {
		// point or spot light (or other kind of light)

		vec3 vertexToLightSource = vec3(light0.position - m * f_coord);
		float distance = length(vertexToLightSource);
		lightDirection = normalize(vertexToLightSource);
		attenuation = 1.0 / (light0.constantAttenuation + light0.linearAttenuation * distance + light0.quadraticAttenuation * distance * distance);

		if (light0.spotCutoff <= 90.0) {
			// spotlight
			float clampedCosine = max(0.0, dot(-lightDirection, light0.spotDirection));

			if (clampedCosine < cos(light0.spotCutoff * 3.14159 / 180.0)) {
				// outside of spotlight cone
				attenuation = 0.0;
			} else {
				attenuation = attenuation * pow(clampedCosine, light0.spotExponent);
			}
		}
	}

	color.rgb = attenuation * vec3(light0.diffuse) * color.rgb * max(0.0, dot(f_normalDirection, lightDirection));

	float z = gl_FragCoord.z / gl_FragCoord.w;
	float fog = min(exp(-fogdensity * z * z), 1.0);

	f_color = mix(fogcolor, color, fog);
}
