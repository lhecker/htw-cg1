#version 330

uniform vec4 lightPosition; // w=0: Spotlight, w=1: Global light
uniform vec3 lightDirection; // only used if the light source is a spotlight

uniform vec3 lightAmbientIntensity; // = vec3(0.6, 0.3, 0);
uniform vec3 lightDiffuseIntensity; // = vec3(1, 0.5, 0);
uniform vec3 lightSpecularIntensity; // = vec3(0, 1, 0);

uniform float lightSpotAttenuationStatic; // = 0.25
uniform float lightSpotAttenuationLinear; // = 0.0
uniform float lightSpotAttenuationCubic; // = 0.01

uniform float lightSpotExponent; // = 50.0 | the lower the value the wider the spotlight

uniform vec3 matAmbientReflectance; // = vec3(1, 1, 1);
uniform vec3 matDiffuseReflectance; // = vec3(1, 1, 1);
uniform vec3 matSpecularReflectance; // = vec3(1, 1, 1);
uniform float matShininess; // = 64;

uniform sampler2D diffuseTexture;

in vec3 f_toLight;
in vec3 f_toCamera;
in vec3 f_normal;
in vec2 f_uv;

out vec4 f_color;


void main(void) {
	// diffuse color of the object from texture
	f_color = texture(diffuseTexture, f_uv);

	// normalize vectors after interpolation
	vec3 L = normalize(f_toLight);
	vec3 V = normalize(f_toCamera);
	vec3 N = normalize(f_normal);
	vec3 D = normalize(-lightDirection);
	float dotNL = lightPosition.w == 1.0 ? dot(N, L) : dot(N, D);

	// get Blinn-Phong reflectance components (ambient, diffuse, specular)

	vec3 Iamb = matAmbientReflectance * lightAmbientIntensity;
	vec3 Idif = vec3(0.0, 0.0, 0.0);
	vec3 Ispe = vec3(0.0, 0.0, 0.0);

	float specularTerm = 0.0;

	if (dotNL > 0.0) {
		float attenuation;

		if (lightPosition.w == 1.0) {
			attenuation = 1.0;
		} else {
			// convert the range [-1, +1] of the cos function to [0, 1]
			float clampedCosine = (dot(L, D) * 0.5) + 0.5;

			float distance = length(f_toLight);
			attenuation = pow(clampedCosine, lightSpotExponent) * (1.0 / (lightSpotAttenuationStatic + lightSpotAttenuationLinear * distance + lightSpotAttenuationCubic * distance * distance));
		}

		// calculate diffuse/specular only if the surface is oriented to the light source

		Idif = attenuation * matDiffuseReflectance * lightDiffuseIntensity * max(0.0, dotNL);

		vec3 H = normalize(L + V); // half vector
		Ispe = attenuation * matSpecularReflectance * lightSpecularIntensity * pow(dot(N, H), matShininess);
	}

	// combination of all components and diffuse color of the object
	f_color.rgb = f_color.rgb * (Iamb + Idif + Ispe);
}