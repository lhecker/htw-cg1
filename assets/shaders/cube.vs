#version 330

// matrices
uniform mat4 m;
uniform mat4 v;
uniform mat4 p;
uniform mat3 normalMatrix;

// position of light and camera
uniform vec3 lightPosition;
uniform vec3 cameraPosition;

// attributes
in vec3 v_coord;
in vec3 v_normal;
in vec2 v_uv;

// data for fragment shader
out vec3 f_normal;
out vec3 f_toLight;
out vec3 f_toCamera;
out vec2 f_uv;

///////////////////////////////////////////////////////////////////

void main(void) {
	// position in world space
	vec4 worldPosition = m * vec4(v_coord, 1);

	// normal in world space
	f_normal = normalize(normalMatrix * v_normal);

	// direction to light
	f_toLight = normalize(lightPosition - worldPosition.xyz);

	// direction to camera
	f_toCamera = normalize(cameraPosition - worldPosition.xyz);

	// texture coordinates to fragment shader
	f_uv = v_uv;

	// screen space coordinates of the vertex
	gl_Position = p * v * worldPosition;
}