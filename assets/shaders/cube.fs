#version 330

// parameters of the light and possible values
uniform vec3 lightAmbientIntensity; // = vec3(0.6, 0.3, 0);
uniform vec3 lightDiffuseIntensity; // = vec3(1, 0.5, 0);
uniform vec3 lightSpecularIntensity; // = vec3(0, 1, 0);

// parameters of the material and possible values
uniform vec3 matAmbientReflectance; // = vec3(1, 1, 1);
uniform vec3 matDiffuseReflectance; // = vec3(1, 1, 1);
uniform vec3 matSpecularReflectance; // = vec3(1, 1, 1);
uniform float matShininess; // = 64;

// texture with diffuse color of the object
uniform sampler2D diffuseTexture;

// data from vertex shader
in vec3 f_normal;
in vec3 f_toLight;
in vec3 f_toCamera;
in vec2 f_uv;

// color for framebuffer
out vec4 f_color;

/////////////////////////////////////////////////////////

// returns intensity of reflected ambient lighting
vec3 ambientLighting() {
	return matAmbientReflectance * lightAmbientIntensity;
}

// returns intensity of diffuse reflection
vec3 diffuseLighting(in vec3 N, in vec3 L) {
	// calculation as for Lambertian reflection
	float diffuseTerm = clamp(dot(N, L), 0, 1) ;
	return matDiffuseReflectance * lightDiffuseIntensity * diffuseTerm;
}

// returns intensity of specular reflection
vec3 specularLighting(in vec3 N, in vec3 L, in vec3 V) {
	float specularTerm = 0;

	// calculate specular reflection only if
	// the surface is oriented to the light source
	if (dot(N, L) > 0.0) {
		// half vector
		vec3 H = normalize(L + V);
		specularTerm = pow(dot(N, H), matShininess);
	}

	return matSpecularReflectance * lightSpecularIntensity * specularTerm;
}

void main(void) {
	// diffuse color of the object from texture
	f_color = texture(diffuseTexture, f_uv);

	if (f_color.a < 0.1) {
		discard;
	}

	// normalize vectors after interpolation
	vec3 L = normalize(f_toLight);
	vec3 V = normalize(f_toCamera);
	vec3 N = normalize(f_normal);

	// get Blinn-Phong reflectance components
	vec3 Iamb = ambientLighting();
	vec3 Idif = diffuseLighting(N, L);
	vec3 Ispe = specularLighting(N, L, V);

	// combination of all components and diffuse color of the object
	f_color.rgb = f_color.rgb * (Iamb + Idif + Ispe);
}