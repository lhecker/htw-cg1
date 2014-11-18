#version 330

// matrices
uniform mat4 m;
uniform mat4 v;
uniform mat4 p;
uniform mat3 normalMatrix;

// position of light and camera
uniform vec4 lightPosition; // w=0: Spotlight, w=1: Global light
uniform vec3 cameraPosition;

// attributes
in vec3 v_coord;
in vec3 v_normal;
in vec2 v_uv;

// data for fragment shader
out vec3 f_toLight;
out vec3 f_toCamera;
out vec3 f_normal;
out vec2 f_uv;

///////////////////////////////////////////////////////////////////

void main(void) {
	// position in world space
	vec4 worldPosition = m * vec4(v_coord, 1);

	// direction to light
	f_toLight = lightPosition.xyz - worldPosition.xyz;

	// direction to camera
	f_toCamera = cameraPosition - worldPosition.xyz;

	// normal in world space
	f_normal = normalMatrix * v_normal;

	// texture coordinates to fragment shader
	f_uv = v_uv;

	// screen space coordinates of the vertex
	gl_Position = p * v * worldPosition;
}