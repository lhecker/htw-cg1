#ifdef _WIN32
# include <windows.h>
# include <io.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <iostream>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <lodepng/picopng.h>

#include "gl_service.h"


// Size of one chunk in blocks
#define CX 16
#define CY 32
#define CZ 16

// Number of chunks in the world
#define SCX 32
#define SCY 2
#define SCZ 32

// Sea level
#define SEALEVEL 4


static GLuint cube_program;
static GLuint white_program;

static GLint cube_uniform_m;
static GLint cube_uniform_v;
static GLint cube_uniform_p;
static GLint cube_uniform_cameraPosition;
static GLint cube_uniform_diffuseTexture;
static GLint cube_uniform_lightAmbientIntensity;
static GLint cube_uniform_lightDiffuseIntensity;
static GLint cube_uniform_lightDirection;
static GLint cube_uniform_lightPosition;
static GLint cube_uniform_lightSpecularIntensity;
static GLint cube_uniform_lightSpotAttenuationCubic;
static GLint cube_uniform_lightSpotAttenuationLinear;
static GLint cube_uniform_lightSpotAttenuationStatic;
static GLint cube_uniform_lightSpotExponent;
static GLint cube_uniform_lightSpotOffset;
static GLint cube_uniform_matAmbientReflectance;
static GLint cube_uniform_matDiffuseReflectance;
static GLint cube_uniform_matShininess;
static GLint cube_uniform_matSpecularReflectance;
static GLint cube_uniform_normalMatrix;

static GLint cube_attribute_coord;
static GLint cube_attribute_normal;
static GLint cube_attribute_uv;

static GLuint textures;

static GLuint box_vao;
static GLuint box_vbo;

static GLuint cursor_vao;
static GLuint cursor_vbo;


static glm::vec3 position;
static glm::vec3 forward;
static glm::vec3 right;
static glm::vec3 up;
static glm::vec3 lookat;
static glm::vec3 angle;

static int ww;
static int wh;

static int mx;
static int my;
static int mz;

static unsigned int face;
static unsigned int buildtype = 1;

static unsigned int keys;

#define M_PIf 3.14159265358979323846f
#define TYPE_TO_UV(type, x, y) glm::i8vec3((x), (y), (type))

/*{ "air", "dirt", "topsoil", "grass", "leaves", "wood", "stone", "sand", "water", "glass", "brick", "ore", "woodrings", "white", "black", "x-y" }*/
static const int transparent[16] = {2, 0, 0, 0, 1, 0, 0, 0, 3, 4, 0, 0, 0, 0, 0, 0};

struct chunk {
	chunk* _left;
	chunk* _right;
	chunk* _below;
	chunk* _above;
	chunk* _front;
	chunk* _back;
	uint8_t _blk[CX][CY][CZ];
	GLuint _vao;
	GLuint _vbo[3];
	int _elements;
	int _ax;
	int _ay;
	int _az;
	bool _changed;
	bool _noised;
	bool _initialized;

	chunk() : _left(0), _right(0), _below(0), _above(0), _front(0), _back(0), _elements(0), _ax(0), _ay(0), _az(0), _changed(true), _noised(false), _initialized(false) {
		glGenBuffers(3, this->_vbo);
		glGenVertexArrays(1, &this->_vao);
		memset(this->_blk, 0, sizeof(this->_blk));
	}

	chunk(int x, int y, int z) : _left(0), _right(0), _below(0), _above(0), _front(0), _back(0), _elements(0), _ax(x), _ay(y), _az(z), _changed(true), _noised(false), _initialized(false) {
		glGenBuffers(3, this->_vbo);
		glGenVertexArrays(1, &this->_vao);
		memset(this->_blk, 0, sizeof(this->_blk));
	}

	~chunk() {
		glDeleteVertexArrays(1, &this->_vao);
		glDeleteBuffers(3, this->_vbo);
	}

	uint8_t get(int x, int y, int z) const {
		if (x < 0) {
			return this->_left ? this->_left->_blk[x + CX][y][z] : 0;
		}

		if (x >= CX) {
			return this->_right ? this->_right->_blk[x - CX][y][z] : 0;
		}

		if (y < 0) {
			return this->_below ? this->_below->_blk[x][y + CY][z] : 0;
		}

		if (y >= CY) {
			return this->_above ? this->_above->_blk[x][y - CY][z] : 0;
		}

		if (z < 0) {
			return this->_front ? this->_front->_blk[x][y][z + CZ] : 0;
		}

		if (z >= CZ) {
			return this->_back ? this->_back->_blk[x][y][z - CZ] : 0;
		}

		return this->_blk[x][y][z];
	}

	bool isblocked(int x1, int y1, int z1, int x2, int y2, int z2) {
		// Invisible blocks are always "blocked"
		if (!this->_blk[x1][y1][z1]) {
			return true;
		}

		// Leaves do not block any other block, including themselves
		if (transparent[this->get(x2, y2, z2)] == 1) {
			return false;
		}

		// Non-transparent blocks always block line of sight
		if (!transparent[this->get(x2, y2, z2)]) {
			return true;
		}

		// Otherwise, LOS is only blocked by blocks if the same transparency type
		return transparent[this->get(x2, y2, z2)] == transparent[this->_blk[x1][y1][z1]];
	}

	void set(int x, int y, int z, uint8_t type) {
		// If coordinates are outside this chunk, find the right one.
		if (x < 0) {
			if (this->_left) {
				this->_left->set(x + CX, y, z, type);
			}

			return;
		}

		if (x >= CX) {
			if (this->_right) {
				this->_right->set(x - CX, y, z, type);
			}

			return;
		}

		if (y < 0) {
			if (this->_below) {
				this->_below->set(x, y + CY, z, type);
			}

			return;
		}

		if (y >= CY) {
			if (this->_above) {
				this->_above->set(x, y - CY, z, type);
			}

			return;
		}

		if (z < 0) {
			if (this->_front) {
				this->_front->set(x, y, z + CZ, type);
			}

			return;
		}

		if (z >= CZ) {
			if (this->_back) {
				this->_back->set(x, y, z - CZ, type);
			}

			return;
		}

		// Change the block
		this->_blk[x][y][z] = type;
		this->_changed = true;

		// When updating blocks at the edge of this chunk,
		// visibility of blocks in the neighbouring chunk might change.
		if (x == 0 && this->_left) {
			this->_left->_changed = true;
		}

		if (x == CX - 1 && this->_right) {
			this->_right->_changed = true;
		}

		if (y == 0 && this->_below) {
			this->_below->_changed = true;
		}

		if (y == CY - 1 && this->_above) {
			this->_above->_changed = true;
		}

		if (z == 0 && this->_front) {
			this->_front->_changed = true;
		}

		if (z == CZ - 1 && this->_back) {
			this->_back->_changed = true;
		}
	}

	static float noise2d(float x, float y, int seed, int octaves, float persistence) {
		float sum = 0;
		float strength = 1.0;
		float scale = 1.0;

		for (int i = 0; i < octaves; i++) {
			sum += strength * glm::simplex(glm::vec2(x, y) * scale);
			scale *= 2.0;
			strength *= persistence;
		}

		return sum;
	}

	static float noise3d_abs(float x, float y, float z, int seed, int octaves, float persistence) {
		float sum = 0;
		float strength = 1.0;
		float scale = 1.0;

		for (int i = 0; i < octaves; i++) {
			sum += strength * fabs(glm::simplex(glm::vec3(x, y, z) * scale));
			scale *= 2.0;
			strength *= persistence;
		}

		return sum;
	}

	void noise(unsigned int seed) {
		if (this->_noised) {
			return;
		}

		this->_noised = true;

		for (int x = 0; x < CX; x++) {
			for (int z = 0; z < CZ; z++) {
				// Land height
				float n = noise2d((x + this->_ax * CX) / 256.0f, (z + this->_az * CZ) / 256.0f, seed, 5, 0.8f) * 4.0f;
				int h = int(n * 2);
				int y = 0;

				// Land blocks
				for (y = 0; y < CY; y++) {
					// Are we above "ground" level?
					if (y + this->_ay * CY >= h) {
						// If we are not yet up to sea level, fill with water blocks
						if (y + this->_ay * CY < SEALEVEL) {
							this->_blk[x][y][z] = 8;
							continue;
							// Otherwise, we are in the air
						} else {
							// A tree!
							if (get(x, y - 1, z) == 3 && (rand() & 0xff) == 0) {
								// Trunk
								h = (rand() & 0x3) + 3;

								for (int i = 0; i < h; i++) {
									set(x, y + i, z, 5);
								}

								// Leaves
								for (int ix = -3; ix <= 3; ix++) {
									for (int iy = -3; iy <= 3; iy++) {
										for (int iz = -3; iz <= 3; iz++) {
											if (ix * ix + iy * iy + iz * iz < 8 + (rand() & 1) && !get(x + ix, y + h + iy, z + iz)) {
												set(x + ix, y + h + iy, z + iz, 4);
											}
										}
									}
								}
							}

							break;
						}
					}

					// Random value used to determine land type
					float r = noise3d_abs((x + this->_ax * CX) / 16.0f, (y + this->_ay * CY) / 16.0f, (z + this->_az * CZ) / 16.0f, seed, 2, 1.0f);

					if (n + r * 5 < 4) {
						// Sand layer
						this->_blk[x][y][z] = 7;
					} else if (n + r * 5 < 8) {
						// Dirt layer, but use grass blocks for the top
						this->_blk[x][y][z] = (h < SEALEVEL || y + this->_ay * CY < h - 1) ? 1 : 3;
					} else if (r < 1.25) {
						// Rock layer
						this->_blk[x][y][z] = 6;
					} else {
						// Sometimes, ores!
						this->_blk[x][y][z] = 11;
					}
				}
			}
		}

		this->_changed = true;
	}

	void update() {
		glm::i8vec3* vertex = new glm::i8vec3[CX * CY * CZ * 18];
		glm::i8vec3* normal = new glm::i8vec3[CX * CY * CZ * 18];
		glm::i8vec3* uv = new glm::i8vec3[CX * CY * CZ * 18];

		size_t i = 0;

		// View from negative x

		for (int x = CX - 1; x >= 0; x--) {
			for (int y = 0; y < CY; y++) {
				for (int z = 0; z < CZ; z++) {
					// Line of sight blocked?
					if (this->isblocked(x, y, z, x - 1, y, z)) {
						continue;
					}

					const uint8_t type = this->_blk[x][y][z];
					uint8_t top = type;
					uint8_t bottom = type;
					uint8_t side = type;

					// Grass block has dirt sides and bottom
					if (top == 3) {
						bottom = 1;
						side = 2;
						// Wood blocks have rings on top and bottom
					} else if (top == 5) {
						top = bottom = 12;
					}

					vertex[i + 0] = glm::i8vec3(x, y, z);
					vertex[i + 1] = glm::i8vec3(x, y, z + 1);
					vertex[i + 2] = glm::i8vec3(x, y + 1, z);
					vertex[i + 3] = glm::i8vec3(x, y + 1, z);
					vertex[i + 4] = glm::i8vec3(x, y, z + 1);
					vertex[i + 5] = glm::i8vec3(x, y + 1, z + 1);

					normal[i + 0] = glm::i8vec3(-1, 0, 0);
					normal[i + 1] = glm::i8vec3(-1, 0, 0);
					normal[i + 2] = glm::i8vec3(-1, 0, 0);
					normal[i + 3] = glm::i8vec3(-1, 0, 0);
					normal[i + 4] = glm::i8vec3(-1, 0, 0);
					normal[i + 5] = glm::i8vec3(-1, 0, 0);

					uv[i + 0] = TYPE_TO_UV(side, 0.0f, 0.0f);
					uv[i + 1] = TYPE_TO_UV(side, 0.0f, 1.0f);
					uv[i + 2] = TYPE_TO_UV(side, 1.0f, 0.0f);
					uv[i + 3] = TYPE_TO_UV(side, 1.0f, 0.0f);
					uv[i + 4] = TYPE_TO_UV(side, 0.0f, 1.0f);
					uv[i + 5] = TYPE_TO_UV(side, 1.0f, 1.0f);

					i += 6;
				}
			}
		}

		// View from positive x

		for (int x = 0; x < CX; x++) {
			for (int y = 0; y < CY; y++) {
				for (int z = 0; z < CZ; z++) {
					if (this->isblocked(x, y, z, x + 1, y, z)) {
						continue;
					}

					const uint8_t type = this->_blk[x][y][z];
					uint8_t top = type;
					uint8_t bottom = type;
					uint8_t side = type;

					if (top == 3) {
						bottom = 1;
						side = 2;
					} else if (top == 5) {
						top = bottom = 12;
					}

					vertex[i + 0] = glm::i8vec3(x + 1, y, z);
					vertex[i + 1] = glm::i8vec3(x + 1, y + 1, z);
					vertex[i + 2] = glm::i8vec3(x + 1, y, z + 1);
					vertex[i + 3] = glm::i8vec3(x + 1, y + 1, z);
					vertex[i + 4] = glm::i8vec3(x + 1, y + 1, z + 1);
					vertex[i + 5] = glm::i8vec3(x + 1, y, z + 1);

					normal[i + 0] = glm::i8vec3(1, 0, 0);
					normal[i + 1] = glm::i8vec3(1, 0, 0);
					normal[i + 2] = glm::i8vec3(1, 0, 0);
					normal[i + 3] = glm::i8vec3(1, 0, 0);
					normal[i + 4] = glm::i8vec3(1, 0, 0);
					normal[i + 5] = glm::i8vec3(1, 0, 0);

					uv[i + 0] = TYPE_TO_UV(side, 0.0f, 0.0f);
					uv[i + 1] = TYPE_TO_UV(side, 1.0f, 0.0f);
					uv[i + 2] = TYPE_TO_UV(side, 0.0f, 1.0f);
					uv[i + 3] = TYPE_TO_UV(side, 1.0f, 0.0f);
					uv[i + 4] = TYPE_TO_UV(side, 1.0f, 1.0f);
					uv[i + 5] = TYPE_TO_UV(side, 0.0f, 1.0f);

					i += 6;
				}
			}
		}

		// View from negative y

		for (int x = 0; x < CX; x++) {
			for (int y = CY - 1; y >= 0; y--) {
				for (int z = 0; z < CZ; z++) {
					if (this->isblocked(x, y, z, x, y - 1, z)) {
						continue;
					}

					const uint8_t type = this->_blk[x][y][z];
					uint8_t top = type;
					uint8_t bottom = type;

					if (top == 3) {
						bottom = 1;
					} else if (top == 5) {
						top = bottom = 12;
					}

					vertex[i + 0] = glm::i8vec3(x, y, z);
					vertex[i + 1] = glm::i8vec3(x + 1, y, z);
					vertex[i + 2] = glm::i8vec3(x, y, z + 1);
					vertex[i + 3] = glm::i8vec3(x + 1, y, z);
					vertex[i + 4] = glm::i8vec3(x + 1, y, z + 1);
					vertex[i + 5] = glm::i8vec3(x, y, z + 1);

					normal[i + 0] = glm::i8vec3(0, -1, 0);
					normal[i + 1] = glm::i8vec3(0, -1, 0);
					normal[i + 2] = glm::i8vec3(0, -1, 0);
					normal[i + 3] = glm::i8vec3(0, -1, 0);
					normal[i + 4] = glm::i8vec3(0, -1, 0);
					normal[i + 5] = glm::i8vec3(0, -1, 0);

					uv[i + 0] = TYPE_TO_UV(bottom, 0.0f, 0.0f);
					uv[i + 1] = TYPE_TO_UV(bottom, 1.0f, 0.0f);
					uv[i + 2] = TYPE_TO_UV(bottom, 0.0f, 1.0f);
					uv[i + 3] = TYPE_TO_UV(bottom, 1.0f, 0.0f);
					uv[i + 4] = TYPE_TO_UV(bottom, 1.0f, 1.0f);
					uv[i + 5] = TYPE_TO_UV(bottom, 0.0f, 1.0f);

					i += 6;
				}
			}
		}

		// View from positive y

		for (int x = 0; x < CX; x++) {
			for (int y = 0; y < CY; y++) {
				for (int z = 0; z < CZ; z++) {
					if (this->isblocked(x, y, z, x, y + 1, z)) {
						continue;
					}

					const uint8_t type = this->_blk[x][y][z];
					uint8_t top = type;
					uint8_t bottom = type;

					if (top == 3) {
						bottom = 1;
					} else if (top == 5) {
						top = bottom = 12;
					}

					vertex[i + 0] = glm::i8vec3(x, y + 1, z);
					vertex[i + 1] = glm::i8vec3(x, y + 1, z + 1);
					vertex[i + 2] = glm::i8vec3(x + 1, y + 1, z);
					vertex[i + 3] = glm::i8vec3(x + 1, y + 1, z);
					vertex[i + 4] = glm::i8vec3(x, y + 1, z + 1);
					vertex[i + 5] = glm::i8vec3(x + 1, y + 1, z + 1);

					normal[i + 0] = glm::i8vec3(0, 1, 0);
					normal[i + 1] = glm::i8vec3(0, 1, 0);
					normal[i + 2] = glm::i8vec3(0, 1, 0);
					normal[i + 3] = glm::i8vec3(0, 1, 0);
					normal[i + 4] = glm::i8vec3(0, 1, 0);
					normal[i + 5] = glm::i8vec3(0, 1, 0);

					uv[i + 0] = TYPE_TO_UV(top, 0.0f, 0.0f);
					uv[i + 1] = TYPE_TO_UV(top, 0.0f, 1.0f);
					uv[i + 2] = TYPE_TO_UV(top, 1.0f, 0.0f);
					uv[i + 3] = TYPE_TO_UV(top, 1.0f, 0.0f);
					uv[i + 4] = TYPE_TO_UV(top, 0.0f, 1.0f);
					uv[i + 5] = TYPE_TO_UV(top, 1.0f, 1.0f);

					i += 6;
				}
			}
		}

		// View from negative z

		for (int x = 0; x < CX; x++) {
			for (int z = CZ - 1; z >= 0; z--) {
				for (int y = 0; y < CY; y++) {
					if (this->isblocked(x, y, z, x, y, z - 1)) {
						continue;
					}

					const uint8_t type = this->_blk[x][y][z];
					uint8_t top = type;
					uint8_t bottom = type;
					uint8_t side = type;

					if (top == 3) {
						bottom = 1;
						side = 2;
					} else if (top == 5) {
						top = bottom = 12;
					}

					vertex[i + 0] = glm::i8vec3(x, y, z);
					vertex[i + 1] = glm::i8vec3(x, y + 1, z);
					vertex[i + 2] = glm::i8vec3(x + 1, y, z);
					vertex[i + 3] = glm::i8vec3(x, y + 1, z);
					vertex[i + 4] = glm::i8vec3(x + 1, y + 1, z);
					vertex[i + 5] = glm::i8vec3(x + 1, y, z);

					normal[i + 0] = glm::i8vec3(0, 0, -1);
					normal[i + 1] = glm::i8vec3(0, 0, -1);
					normal[i + 2] = glm::i8vec3(0, 0, -1);
					normal[i + 3] = glm::i8vec3(0, 0, -1);
					normal[i + 4] = glm::i8vec3(0, 0, -1);
					normal[i + 5] = glm::i8vec3(0, 0, -1);

					uv[i + 0] = TYPE_TO_UV(side, 0.0f, 0.0f);
					uv[i + 1] = TYPE_TO_UV(side, 0.0f, 1.0f);
					uv[i + 2] = TYPE_TO_UV(side, 1.0f, 0.0f);
					uv[i + 3] = TYPE_TO_UV(side, 0.0f, 1.0f);
					uv[i + 4] = TYPE_TO_UV(side, 1.0f, 1.0f);
					uv[i + 5] = TYPE_TO_UV(side, 1.0f, 0.0f);

					i += 6;
				}
			}
		}

		// View from positive z

		for (int x = 0; x < CX; x++) {
			for (int z = 0; z < CZ; z++) {
				for (int y = 0; y < CY; y++) {
					if (this->isblocked(x, y, z, x, y, z + 1)) {
						continue;
					}

					const uint8_t type = this->_blk[x][y][z];
					uint8_t top = type;
					uint8_t bottom = type;
					uint8_t side = type;

					if (top == 3) {
						bottom = 1;
						side = 2;
					} else if (top == 5) {
						top = bottom = 12;
					}

					vertex[i + 0] = glm::i8vec3(x, y, z + 1);
					vertex[i + 1] = glm::i8vec3(x + 1, y, z + 1);
					vertex[i + 2] = glm::i8vec3(x, y + 1, z + 1);
					vertex[i + 3] = glm::i8vec3(x, y + 1, z + 1);
					vertex[i + 4] = glm::i8vec3(x + 1, y, z + 1);
					vertex[i + 5] = glm::i8vec3(x + 1, y + 1, z + 1);

					normal[i + 0] = glm::i8vec3(0, 0, 1);
					normal[i + 1] = glm::i8vec3(0, 0, 1);
					normal[i + 2] = glm::i8vec3(0, 0, 1);
					normal[i + 3] = glm::i8vec3(0, 0, 1);
					normal[i + 4] = glm::i8vec3(0, 0, 1);
					normal[i + 5] = glm::i8vec3(0, 0, 1);

					uv[i + 0] = TYPE_TO_UV(side, 0.0f, 0.0f);
					uv[i + 1] = TYPE_TO_UV(side, 1.0f, 0.0f);
					uv[i + 2] = TYPE_TO_UV(side, 0.0f, 1.0f);
					uv[i + 3] = TYPE_TO_UV(side, 0.0f, 1.0f);
					uv[i + 4] = TYPE_TO_UV(side, 1.0f, 0.0f);
					uv[i + 5] = TYPE_TO_UV(side, 1.0f, 1.0f);

					i += 6;
				}
			}
		}

		this->_changed = false;
		this->_elements = i;

		if (this->_elements) {
			glBindVertexArray(this->_vao);


			glBindBuffer(GL_ARRAY_BUFFER, this->_vbo[0]);
			glBufferData(GL_ARRAY_BUFFER, i * sizeof(vertex[0]), vertex, GL_STATIC_DRAW);

			glEnableVertexAttribArray(cube_attribute_coord);
			glVertexAttribPointer(cube_attribute_coord, 3, GL_BYTE, GL_FALSE, 0, 0);


			glBindBuffer(GL_ARRAY_BUFFER, this->_vbo[1]);
			glBufferData(GL_ARRAY_BUFFER, i * sizeof(normal[0]), normal, GL_STATIC_DRAW);

			glEnableVertexAttribArray(cube_attribute_normal);
			glVertexAttribPointer(cube_attribute_normal, 3, GL_BYTE, GL_FALSE, 0, 0);


			glBindBuffer(GL_ARRAY_BUFFER, this->_vbo[2]);
			glBufferData(GL_ARRAY_BUFFER, i * sizeof(uv[0]), uv, GL_STATIC_DRAW);

			glEnableVertexAttribArray(cube_attribute_uv);
			glVertexAttribPointer(cube_attribute_uv, 3, GL_BYTE, GL_FALSE, 0, 0);
		}

		delete[] vertex;
		delete[] normal;
		delete[] uv;
	}

	void render() {
		if (this->_changed) {
			update();
		}

		if (this->_elements) {
			glBindVertexArray(this->_vao);
			glDrawArrays(GL_TRIANGLES, 0, this->_elements);
		}
	}
};

struct superchunk {
	chunk* _c[SCX][SCY][SCZ];
	unsigned int _seed;

	superchunk() {
		this->_seed = (unsigned int)time(NULL);

		for (int x = 0; x < SCX; x++) {
			for (int y = 0; y < SCY; y++) {
				for (int z = 0; z < SCZ; z++) {
					this->_c[x][y][z] = new chunk(x - SCX / 2, y - SCY / 2, z - SCZ / 2);
				}
			}
		}

		for (int x = 0; x < SCX; x++) {
			for (int y = 0; y < SCY; y++) {
				for (int z = 0; z < SCZ; z++) {
					if (x > 0) {
						this->_c[x][y][z]->_left = this->_c[x - 1][y][z];
					}

					if (x < SCX - 1) {
						this->_c[x][y][z]->_right = this->_c[x + 1][y][z];
					}

					if (y > 0) {
						this->_c[x][y][z]->_below = this->_c[x][y - 1][z];
					}

					if (y < SCY - 1) {
						this->_c[x][y][z]->_above = this->_c[x][y + 1][z];
					}

					if (z > 0) {
						this->_c[x][y][z]->_front = this->_c[x][y][z - 1];
					}

					if (z < SCZ - 1) {
						this->_c[x][y][z]->_back = this->_c[x][y][z + 1];
					}
				}
			}
		}
	}

	uint8_t get(int x, int y, int z) const {
		int cx = (x + CX * (SCX / 2)) / CX;
		int cy = (y + CY * (SCY / 2)) / CY;
		int cz = (z + CZ * (SCZ / 2)) / CZ;

		if (cx < 0 || cx >= SCX || cy < 0 || cy >= SCY || cz <= 0 || cz >= SCZ) {
			return 0;
		}

		return this->_c[cx][cy][cz]->get(x & (CX - 1), y & (CY - 1), z & (CZ - 1));
	}

	void set(int x, int y, int z, uint8_t type) {
		int cx = (x + CX * (SCX / 2)) / CX;
		int cy = (y + CY * (SCY / 2)) / CY;
		int cz = (z + CZ * (SCZ / 2)) / CZ;

		if (cx < 0 || cx >= SCX || cy < 0 || cy >= SCY || cz <= 0 || cz >= SCZ) {
			return;
		}

		this->_c[cx][cy][cz]->set(x & (CX - 1), y & (CY - 1), z & (CZ - 1), type);
	}

	void render(const glm::mat4& v, const glm::mat4& p) {
		float ud = std::numeric_limits<float>::infinity();
		int ux = -1;
		int uy = -1;
		int uz = -1;

		for (int x = 0; x < SCX; x++) {
			for (int y = 0; y < SCY; y++) {
				for (int z = 0; z < SCZ; z++) {
					glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(this->_c[x][y][z]->_ax * CX, this->_c[x][y][z]->_ay * CY, this->_c[x][y][z]->_az * CZ));
					glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(m));

					// Is this chunk on the screen?
					glm::vec4 center = p * v * m * glm::vec4(CX / 2, CY / 2, CZ / 2, 1);

					float d = glm::length(center);
					center.x /= center.w;
					center.y /= center.w;

					// If it is behind the camera, don't bother drawing it
					if (center.z < -CY / 2) {
						continue;
					}

					// If it is outside the screen, don't bother drawing it
					if (fabsf(center.x) > 1 + fabsf(CY * 2 / center.w) || fabsf(center.y) > 1 + fabsf(CY * 2 / center.w)) {
						continue;
					}

					// If this chunk is not initialized, skip it
					if (!this->_c[x][y][z]->_initialized) {
						// But if it is the closest to the camera, mark it for initialization
						if (ux < 0 || d < ud) {
							ud = d;
							ux = x;
							uy = y;
							uz = z;
						}

						continue;
					}

					glUniformMatrix4fv(cube_uniform_m, 1, GL_FALSE, glm::value_ptr(m));
					glUniformMatrix3fv(cube_uniform_normalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));

					this->_c[x][y][z]->render();
				}
			}
		}

		if (ux >= 0) {
			this->_c[ux][uy][uz]->noise(this->_seed);

			if (this->_c[ux][uy][uz]->_left) {
				this->_c[ux][uy][uz]->_left->noise(this->_seed);
			}

			if (this->_c[ux][uy][uz]->_right) {
				this->_c[ux][uy][uz]->_right->noise(this->_seed);
			}

			if (this->_c[ux][uy][uz]->_below) {
				this->_c[ux][uy][uz]->_below->noise(this->_seed);
			}

			if (this->_c[ux][uy][uz]->_above) {
				this->_c[ux][uy][uz]->_above->noise(this->_seed);
			}

			if (this->_c[ux][uy][uz]->_front) {
				this->_c[ux][uy][uz]->_front->noise(this->_seed);
			}

			if (this->_c[ux][uy][uz]->_back) {
				this->_c[ux][uy][uz]->_back->noise(this->_seed);
			}

			this->_c[ux][uy][uz]->_initialized = true;
		}
	}
};

static superchunk* world;

// Calculate the forward, right and lookat vectors from the angle vector
static void update_vectors() {
	forward.x = sinf(angle.x);
	forward.y = 0;
	forward.z = cosf(angle.x);

	right.x = -cosf(angle.x);
	right.y = 0;
	right.z = sinf(angle.x);

	lookat.x = sinf(angle.x) * cosf(angle.y);
	lookat.y = sinf(angle.y);
	lookat.z = cosf(angle.x) * cosf(angle.y);

	up = glm::cross(right, lookat);
}


static int init_resources() {
	// Create shaders
	cube_program = gl_service::program_from_file("assets/shaders/cube.vs", "assets/shaders/cube.fs");
	white_program = gl_service::program_from_file("assets/shaders/white.vs", "assets/shaders/white.fs");

	if (cube_program == 0 || white_program == 0) {
		return 1;
	}

	cube_uniform_m                          = glGetUniformLocation(cube_program, "m");
	cube_uniform_v                          = glGetUniformLocation(cube_program, "v");
	cube_uniform_p                          = glGetUniformLocation(cube_program, "p");
	cube_uniform_normalMatrix               = glGetUniformLocation(cube_program, "normalMatrix");
	cube_uniform_cameraPosition             = glGetUniformLocation(cube_program, "cameraPosition");
	cube_uniform_lightPosition              = glGetUniformLocation(cube_program, "lightPosition");
	cube_uniform_lightDirection             = glGetUniformLocation(cube_program, "lightDirection");
	cube_uniform_lightAmbientIntensity      = glGetUniformLocation(cube_program, "lightAmbientIntensity");
	cube_uniform_lightDiffuseIntensity      = glGetUniformLocation(cube_program, "lightDiffuseIntensity");
	cube_uniform_lightSpecularIntensity     = glGetUniformLocation(cube_program, "lightSpecularIntensity");
	cube_uniform_lightSpotAttenuationStatic = glGetUniformLocation(cube_program, "lightSpotAttenuationStatic");
	cube_uniform_lightSpotAttenuationLinear = glGetUniformLocation(cube_program, "lightSpotAttenuationLinear");
	cube_uniform_lightSpotAttenuationCubic  = glGetUniformLocation(cube_program, "lightSpotAttenuationCubic");
	cube_uniform_lightSpotExponent          = glGetUniformLocation(cube_program, "lightSpotExponent");
	cube_uniform_lightSpotOffset            = glGetUniformLocation(cube_program, "lightSpotOffset");
	cube_uniform_matAmbientReflectance      = glGetUniformLocation(cube_program, "matAmbientReflectance");
	cube_uniform_matDiffuseReflectance      = glGetUniformLocation(cube_program, "matDiffuseReflectance");
	cube_uniform_matSpecularReflectance     = glGetUniformLocation(cube_program, "matSpecularReflectance");
	cube_uniform_matShininess               = glGetUniformLocation(cube_program, "matShininess");
	cube_uniform_diffuseTexture             = glGetUniformLocation(cube_program, "diffuseTexture");
	cube_attribute_coord                    = glGetAttribLocation(cube_program, "v_coord");
	cube_attribute_normal                   = glGetAttribLocation(cube_program, "v_normal");
	cube_attribute_uv                       = glGetAttribLocation(cube_program, "v_uv");

	if (   cube_uniform_m == -1
	    || cube_uniform_v == -1
	    || cube_uniform_p == -1
	    || cube_uniform_normalMatrix == -1
	    || cube_uniform_cameraPosition == -1
	    || cube_uniform_lightPosition == -1
	    || cube_uniform_lightDirection == -1
	    || cube_uniform_lightAmbientIntensity == -1
	    || cube_uniform_lightDiffuseIntensity == -1
	    || cube_uniform_lightSpecularIntensity == -1
	    || cube_uniform_lightSpotAttenuationStatic == -1
	    || cube_uniform_lightSpotAttenuationLinear == -1
	    || cube_uniform_lightSpotAttenuationCubic == -1
	    || cube_uniform_lightSpotExponent == -1
	    || cube_uniform_lightSpotOffset == -1
	    || cube_uniform_matAmbientReflectance == -1
	    || cube_uniform_matDiffuseReflectance == -1
	    || cube_uniform_matSpecularReflectance == -1
	    || cube_uniform_matShininess == -1
	    || cube_uniform_diffuseTexture == -1
	    || cube_attribute_coord == -1
	    || cube_attribute_normal == -1
		|| cube_attribute_uv == -1)
	{
		return 2;
	}

	glUseProgram(cube_program);
	glUniform3fv(cube_uniform_lightAmbientIntensity, 1, glm::value_ptr(glm::vec3(0.1f, 0.1f, 0.1f)));
	glUniform3fv(cube_uniform_lightDiffuseIntensity, 1, glm::value_ptr(glm::vec3(0.8f, 0.8f, 0.6f)));
	glUniform3fv(cube_uniform_lightSpecularIntensity, 1, glm::value_ptr(glm::vec3(0.4f, 0.4f, 0.4f)));
	glUniform1f(cube_uniform_lightSpotAttenuationStatic, 0.2f);
	glUniform1f(cube_uniform_lightSpotAttenuationLinear, 0.0f);
	glUniform1f(cube_uniform_lightSpotAttenuationCubic, 0.005f);
	glUniform1f(cube_uniform_lightSpotExponent, 20.0f);
	glUniform1f(cube_uniform_lightSpotOffset, 0.0f);
	glUniform3fv(cube_uniform_matAmbientReflectance, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
	glUniform3fv(cube_uniform_matDiffuseReflectance, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
	glUniform3fv(cube_uniform_matSpecularReflectance, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
	glUniform1f(cube_uniform_matShininess, 1.0f);


	// Create and upload the texture
	std::vector<uint8_t> imageData = gl_service::load_file("assets/textures/textures.png");
	std::vector<uint8_t> image;
	unsigned long imageWidth;
	unsigned long imageHeight;

	if (lodepng::decodePNG(image, imageWidth, imageHeight, imageData.data(), imageData.size())) {
		return 3;
	}

	// the last test of those 3 checks if the value is a multiple of 2
	if (imageWidth == 0 || (imageWidth & (imageWidth - 1)) != 0 || imageWidth > imageHeight || imageHeight % imageWidth != 0) {
		return 4;
	}

	const uint32_t* data = (uint32_t*)image.data();
	const unsigned long spritePixelCount = imageWidth * imageWidth;
	const unsigned long layers = imageHeight / imageWidth;

	glGenTextures(1, &textures);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textures);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, imageWidth, imageWidth, layers);

	for (unsigned long i = 0; i < layers; i++) {
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, imageWidth, imageWidth, layers, GL_RGBA, GL_UNSIGNED_BYTE, data + (i * spritePixelCount * 4));
	}


	world = new superchunk;

	position = glm::vec3(0, SEALEVEL + 10, 0);
	angle = glm::vec3(0, -0.5, 0);
	update_vectors();


	glGenVertexArrays(1, &box_vao);
	glGenBuffers(1, &box_vbo);

	// Create a VBO for the cursor
	float cross[4][2] = {
		{ -0.05f,  0.00f }, { +0.05f,  0.00f },
		{  0.00f, -0.05f }, {  0.00f, +0.05f },
	};

	glGenVertexArrays(1, &cursor_vao);
	glBindVertexArray(cursor_vao);

	glGenBuffers(1, &cursor_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cursor_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cross), cross, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	return 0;
}

// Not really GLSL fract(), but the absolute distance to the nearest integer value
static float fract(float value) {
	float f = value - floorf(value);

	if (f > 0.5f) {
		return 1.0f - f;
	} else {
		return f;
	}
}

int main(int argc, char* argv[]) {
	srand(time(nullptr));

	gl_service service("minecraft");
	service.set_cursor_disabled(true);

	service.on_reshape([](int width, int height) {
		ww = width;
		wh = height;
	});

	service.on_keydown([](int key) {
		switch (key) {
		case GLFW_KEY_A:
			keys |= 1;
			break;

		case GLFW_KEY_D:
			keys |= 2;
			break;

		case GLFW_KEY_W:
			keys |= 4;
			break;

		case GLFW_KEY_S:
			keys |= 8;
			break;

		case GLFW_KEY_SPACE:
			keys |= 16;
			break;

		case GLFW_KEY_LEFT_CONTROL:
			keys |= 32;
			break;

		case GLFW_KEY_1:
			position = glm::vec3(0, CY + 1, 0);
			angle = glm::vec3(0, -0.5, 0);
			update_vectors();
			break;

		case GLFW_KEY_2:
			position = glm::vec3(0, CX * SCX, 0);
			angle = glm::vec3(0, -M_PIf / 2, 0);
			update_vectors();
			break;
		}
	});

	service.on_keyup([](int key) {
		switch (key) {
		case GLFW_KEY_A:
			keys &= ~1;
			break;

		case GLFW_KEY_D:
			keys &= ~2;
			break;

		case GLFW_KEY_W:
			keys &= ~4;
			break;

		case GLFW_KEY_S:
			keys &= ~8;
			break;

		case GLFW_KEY_SPACE:
			keys &= ~16;
			break;

		case GLFW_KEY_LEFT_CONTROL:
			keys &= ~32;
			break;
		}
	});

	service.on_mousedown([](int button) {
		if (button == 0) {
			if (face == 0) {
				mx++;
			}

			if (face == 3) {
				mx--;
			}

			if (face == 1) {
				my++;
			}

			if (face == 4) {
				my--;
			}

			if (face == 2) {
				mz++;
			}

			if (face == 5) {
				mz--;
			}

			world->set(mx, my, mz, buildtype);
		} else {
			world->set(mx, my, mz, 0);
		}
	});

	service.on_scroll([](float xoffset, float yoffset) {
		if (yoffset < 0.0) {
			buildtype--;
		} else {
			buildtype++;
		}

		buildtype &= 0xf;
	});

	service.on_mousemoved([&service](float xpos, float ypos) {
		static const float mousespeed = 0.002f;

		angle.x -= xpos * mousespeed;
		angle.y -= ypos * mousespeed;

		if (angle.x < -M_PIf) {
			angle.x += M_PIf * 2;
		}

		if (angle.x > M_PIf) {
			angle.x -= M_PIf * 2;
		}

		if (angle.y < -M_PIf / 2) {
			angle.y = -M_PIf / 2;
		}

		if (angle.y > M_PIf / 2) {
			angle.y = M_PIf / 2;
		}

		update_vectors();

		service.set_cursor_position(0.0, 0.0);
	});

	service.on_display([](float delta) {
		static const float movespeed = 10;
		float dt = delta;

		if (keys & 1) {
			position -= right * movespeed * dt;
		}

		if (keys & 2) {
			position += right * movespeed * dt;
		}

		if (keys & 4) {
			position += forward * movespeed * dt;
		}

		if (keys & 8) {
			position -= forward * movespeed * dt;
		}

		if (keys & 16) {
			position.y += movespeed * dt;
		}

		if (keys & 32) {
			position.y -= movespeed * dt;
		}


		glClearColor(0.0f, 0.2f, 0.4f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		glUseProgram(cube_program);

		glm::mat4 v = glm::lookAt(position, position + lookat, up);
		glm::mat4 p = glm::perspective(45.0f, float(ww) / float(wh), 0.01f, 1000.0f);

		glUniformMatrix4fv(cube_uniform_v, 1, GL_FALSE, glm::value_ptr(v));
		glUniformMatrix4fv(cube_uniform_p, 1, GL_FALSE, glm::value_ptr(p));

		glUniform3fv(cube_uniform_cameraPosition, 1, glm::value_ptr(position));
		glUniform4fv(cube_uniform_lightPosition, 1, glm::value_ptr(glm::vec4(position, 0.0f)));
		glUniform3fv(cube_uniform_lightDirection, 1, glm::value_ptr(lookat));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, textures);
		glUniform1i(cube_uniform_diffuseTexture, /*GL_TEXTURE*/0);

		world->render(v, p);

		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		// Find out coordinates of the center pixel
		float depth;
		glReadPixels(ww / 2, wh / 2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

		// Find out which block it belongs to
		glm::vec4 viewport = glm::vec4(0, 0, ww, wh);
		glm::vec3 wincoord = glm::vec3(ww / 2, wh / 2, depth);
		glm::vec3 objcoord = glm::unProject(wincoord, v, p, viewport);

		mx = int(objcoord.x);
		my = int(objcoord.y);
		mz = int(objcoord.z);

		if (objcoord.x < 0) {
			mx--;
		}

		if (objcoord.y < 0) {
			my--;
		}

		if (objcoord.z < 0) {
			mz--;
		}

		// Find out which face of the block we are looking at
		if (fract(objcoord.x) < fract(objcoord.y)) {
			if (fract(objcoord.x) < fract(objcoord.z)) {
				mx--;
				face = 0;    // X
			} else {
				face = 2;    // Z
			}
		} else if (fract(objcoord.y) < fract(objcoord.z)) {
			face = 1;    // Y
		} else {
			face = 2;    // Z
		}

		if (face == 0 && lookat.x > 0) {
			mx++;
			face = 3;
		}

		if (face == 1 && lookat.y > 0) {
			my++;
			face = 4;
		}

		if (face == 2 && lookat.z > 0) {
			mz++;
			face = 5;
		}

		const float mxf = float(mx);
		const float myf = float(my);
		const float mzf = float(mz);

		// Render a box around the block we are pointing at.
		float box[24][4] = {
			{mxf + 0.0f, myf + 0.0f, mzf + 0.0f, 14},
			{mxf + 1.0f, myf + 0.0f, mzf + 0.0f, 14},
			{mxf + 0.0f, myf + 1.0f, mzf + 0.0f, 14},
			{mxf + 1.0f, myf + 1.0f, mzf + 0.0f, 14},
			{mxf + 0.0f, myf + 0.0f, mzf + 1.0f, 14},
			{mxf + 1.0f, myf + 0.0f, mzf + 1.0f, 14},
			{mxf + 0.0f, myf + 1.0f, mzf + 1.0f, 14},
			{mxf + 1.0f, myf + 1.0f, mzf + 1.0f, 14},

			{mxf + 0.0f, myf + 0.0f, mzf + 0.0f, 14},
			{mxf + 0.0f, myf + 1.0f, mzf + 0.0f, 14},
			{mxf + 1.0f, myf + 0.0f, mzf + 0.0f, 14},
			{mxf + 1.0f, myf + 1.0f, mzf + 0.0f, 14},
			{mxf + 0.0f, myf + 0.0f, mzf + 1.0f, 14},
			{mxf + 0.0f, myf + 1.0f, mzf + 1.0f, 14},
			{mxf + 1.0f, myf + 0.0f, mzf + 1.0f, 14},
			{mxf + 1.0f, myf + 1.0f, mzf + 1.0f, 14},

			{mxf + 0.0f, myf + 0.0f, mzf + 0.0f, 14},
			{mxf + 0.0f, myf + 0.0f, mzf + 1.0f, 14},
			{mxf + 1.0f, myf + 0.0f, mzf + 0.0f, 14},
			{mxf + 1.0f, myf + 0.0f, mzf + 1.0f, 14},
			{mxf + 0.0f, myf + 1.0f, mzf + 0.0f, 14},
			{mxf + 0.0f, myf + 1.0f, mzf + 1.0f, 14},
			{mxf + 1.0f, myf + 1.0f, mzf + 0.0f, 14},
			{mxf + 1.0f, myf + 1.0f, mzf + 1.0f, 14},
		};

		glDisable(GL_CULL_FACE);

		glBindVertexArray(box_vao);

		glBindBuffer(GL_ARRAY_BUFFER, box_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_STATIC_DRAW);

		glEnableVertexAttribArray(cube_attribute_coord);
		glVertexAttribPointer(cube_attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

		glm::mat4 m = glm::mat4(1.0f);
		glUniformMatrix4fv(cube_uniform_m, 1, GL_FALSE, glm::value_ptr(m));
		glDrawArrays(GL_LINES, 0, 24);


		// Draw a cross in the center of the screen
		glUseProgram(white_program);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);

		glBindVertexArray(cursor_vao);
		glDrawArrays(GL_LINES, 0, 4);

		glDisable(GL_BLEND);
	});

	try {
		int r = init_resources();
		if (r) {
			return r;
		}
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 127;
	}

	service.run();
	return 0;
}

#ifdef _WIN32

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	std::ofstream log("glcraft.log");
	std::cout.rdbuf(log.rdbuf());
	std::cerr.rdbuf(log.rdbuf());

	return main(0, nullptr);
}

#endif
