#include "gl_service.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>


static std::string get_log(GLuint object) {
	GLint log_length = 0;

	if (glIsShader(object)) {
		glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
	} else if (glIsProgram(object)) {
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
	} else {
		return "Not a shader, nor a program!";
	}

	std::string log(log_length, ' ');

	if (glIsShader(object)) {
		glGetShaderInfoLog(object, log.size(), NULL, (char*)log.data());
	} else {
		glGetProgramInfoLog(object, log.size(), NULL, (char*)log.data());
	}

	return log;
}


gl_service::gl_service(const std::string& title) : _has_focus(true) {
	if (glfwInit() != GL_TRUE) {
		throw std::runtime_error("glfwInit() != GL_TRUE");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	this->_window = glfwCreateWindow(640, 480, title.c_str(), nullptr, nullptr);

	if (!this->_window) {
		glfwTerminate();
		throw std::runtime_error("glfwCreateWindow() returned nullptr");
	}

	glfwSetWindowUserPointer(this->_window, this);
	glfwMakeContextCurrent(this->_window);

	glewExperimental = GL_TRUE;

	if (glewInit() != GLEW_OK) {
		throw std::runtime_error("glewInit() != GLEW_OK");
	}
}

gl_service::~gl_service() {
}

void gl_service::run() {
	glfwSwapInterval(1);

	glfwSetWindowFocusCallback(this->_window, [](GLFWwindow* window, int got_focus) {
		auto self = reinterpret_cast<gl_service*>(glfwGetWindowUserPointer(window));
		self->_has_focus = got_focus != 0;
	});

	glfwSetFramebufferSizeCallback(this->_window, [](GLFWwindow* window, int width, int height) {
		auto self = reinterpret_cast<gl_service*>(glfwGetWindowUserPointer(window));
		glViewport(0, 0, width, height);
		self->emit_reshape_s(width, height);
	});

	glfwSetWindowRefreshCallback(this->_window, [](GLFWwindow* window) {
		auto self = reinterpret_cast<gl_service*>(glfwGetWindowUserPointer(window));

		float t = float(glfwGetTime());
		self->emit_display_s(t - self->_time);
		self->_time = t;

		glfwSwapBuffers(window);
	});

	glfwSetKeyCallback(this->_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		auto self = reinterpret_cast<gl_service*>(glfwGetWindowUserPointer(window));

		switch (action) {
		case GLFW_RELEASE:
			if (key == GLFW_KEY_ESCAPE && self->_curser_disabled) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}

			self->emit_keyup_s(key);
			break;
		case GLFW_PRESS:
			// if GLFW_CURSOR_DISABLED is set, Alt+F4 won't work anymore in GLFW 3.0.4
#ifdef _WIN32
			if (self->_curser_disabled && key == GLFW_KEY_F4 && mods == GLFW_MOD_ALT) {
				glfwSetWindowShouldClose(window, 1);
			}
#endif
			self->emit_keydown_s(key);
			break;
		}
	});

	glfwSetMouseButtonCallback(this->_window, [](GLFWwindow* window, int button, int action, int mods) {
		auto self = reinterpret_cast<gl_service*>(glfwGetWindowUserPointer(window));

		switch (action) {
			case GLFW_RELEASE:
				if (self->_curser_disabled) {
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				}

				self->emit_mouseup_s(button);
				break;
			case GLFW_PRESS:
				self->emit_mousedown_s(button);
				break;
		}
	});

	glfwSetCursorPosCallback(this->_window, [](GLFWwindow* window, double xpos, double ypos) {
		auto self = reinterpret_cast<gl_service*>(glfwGetWindowUserPointer(window));

		if (self->_has_focus && (!self->_curser_disabled || glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)) {
			self->emit_mousemoved_s(float(xpos), float(ypos));
		}
	});

	glfwSetScrollCallback(this->_window, [](GLFWwindow* window, double xoffset, double yoffset) {
		auto self = reinterpret_cast<gl_service*>(glfwGetWindowUserPointer(window));
		self->emit_scroll_s(float(xoffset), float(yoffset));
	});

	{
		int width;
		int height;
		glfwGetFramebufferSize(this->_window, &width, &height);
		this->emit_reshape_s(width, height);
	}

	this->_time = float(glfwGetTime());

	while (!glfwWindowShouldClose(this->_window)) {
		float t = float(glfwGetTime());
		this->emit_display_s(t - this->_time);
		this->_time = t;

		glfwSwapBuffers(this->_window);

		glfwPollEvents();

		// throttle if the window is not visible etc.
		while (!_has_focus) {
			glfwWaitEvents();
		}
	}

	glfwTerminate();
}

float gl_service::time() const {
	return this->_time;
}

void gl_service::set_cursor_disabled(bool disabled) {
	this->_curser_disabled = disabled;

	if (disabled && this->_has_focus) {
		glfwSetInputMode(this->_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
}

void gl_service::set_cursor_position(float xpos, float ypos) {
	glfwSetCursorPos(this->_window, xpos, ypos);
}


std::vector<unsigned char> gl_service::load_file(const std::string& path) {
	std::ifstream fd(path, std::ios::binary);

	if (!fd) {
		throw std::runtime_error("Failed to open file!");
	}

	fd.seekg(0, std::ios::end);
	std::streamoff len = fd.tellg();
	fd.seekg(0);

	std::vector<unsigned char> vec((size_t)(len));
	fd.read((char*)vec.data(), vec.size());

	return vec;
}

// vs = vertex shader | fs = fragment shader
GLuint gl_service::program_from_file(const std::string& vsPath, const std::string& fsPath) {
	auto vs = gl_service::load_file(vsPath);
	auto fs = gl_service::load_file(fsPath);
	return gl_service::program_from_source(vs.data(), vs.size(), fs.data(), fs.size());
}

GLuint gl_service::program_from_source(const uint8_t* vsData, const size_t vsSize, const uint8_t* fsData, const size_t fsSize) {
	auto loadShaderFromFile = [](GLenum shaderType, const uint8_t* data, const size_t size) -> GLuint {
		if (!data) {
			throw std::invalid_argument("Invalid pointer!");
		}

		if (size > (size_t)std::numeric_limits<GLint>::max()) {
			throw std::invalid_argument("Shader file too large!");
		}

		GLuint shaderId = glCreateShader(shaderType);
		glShaderSource(shaderId, 1, (const GLchar**)&data, (GLint*)&size);
		glCompileShader(shaderId);

		GLint result;

		/*
		 * Returns GL_TRUE if the last compile operation on shader
		 * was successful and GL_FALSE otherwise.
		 */
		glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);

		if (result == GL_FALSE) {
			throw std::runtime_error(get_log(shaderId));
		}

		return shaderId;
	};

	auto linkShaders = [](GLuint vsId, GLuint fsId) -> GLuint {
		GLuint programId = glCreateProgram();
		glAttachShader(programId, vsId);
		glAttachShader(programId, fsId);
		glLinkProgram(programId);

		// same as in loadShaderFromFile()
		GLint result;

		glGetProgramiv(programId, GL_LINK_STATUS, &result);

		if (result == GL_FALSE) {
			throw std::runtime_error(get_log(programId));
		}

		glDeleteShader(vsId);
		glDeleteShader(fsId);

		return programId;
	};


	GLuint vsId = loadShaderFromFile(GL_VERTEX_SHADER, vsData, vsSize);
	GLuint fsId = loadShaderFromFile(GL_FRAGMENT_SHADER, fsData, fsSize);
	return linkShaders(vsId, fsId);
}
