#ifndef gl_service_h
#define gl_service_h

#define GLFW_INCLUDE_NONE

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>


#define GLFW_ADD_CALLBACK(vis, name, ret, ...)                    \
	vis:                                                          \
		typedef std::function<ret(__VA_ARGS__)> on_##name##_t;    \
		                                                          \
	private:                                                      \
		on_##name##_t _on_##name;                                 \
		                                                          \
	protected:                                                    \
		template<typename... nacArgs>                             \
		ret emit_##name(nacArgs&&... nacargs) {                   \
			return _on_##name(std::forward<nacArgs>(nacargs)...); \
		}                                                         \
		                                                          \
		template<typename... nacArgs>                             \
		bool emit_##name##_s(nacArgs&&... nacargs) {              \
			if (_on_##name) {                                     \
				_on_##name(std::forward<nacArgs>(nacargs)...);    \
				return true;                                      \
			}                                                     \
			return false;                                         \
		}                                                         \
		                                                          \
	vis:                                                          \
		template<typename nacF>                                   \
		void on_##name(nacF&& nacf) {                             \
			_on_##name = std::forward<nacF>(nacf);                \
		}                                                         \
		                                                          \
		bool has_##name##_callback() const {                      \
			return static_cast<bool>(_on_##name);                 \
		}


class gl_service {
	GLFW_ADD_CALLBACK(public, reshape, void, int width, int height)
	GLFW_ADD_CALLBACK(public, display, void, float delta)
	GLFW_ADD_CALLBACK(public, keydown, void, int key)
	GLFW_ADD_CALLBACK(public, keyup, void, int key)
	GLFW_ADD_CALLBACK(public, mousedown, void, int button)
	GLFW_ADD_CALLBACK(public, mouseup, void, int button)
	GLFW_ADD_CALLBACK(public, mousemoved, void, float xpos, float ypos)
	GLFW_ADD_CALLBACK(public, scroll, void, float xoffset, float yoffset)

public:
	gl_service(const std::string& title);
	~gl_service();

	void run();
	float time() const;
	void set_cursor_disabled(bool disabled);
	void set_cursor_position(float xpos, float ypos);

	static std::vector<uint8_t> load_file(const std::string& path);
	static GLuint program_from_file(const std::string& vsPath, const std::string& fsPath);
	static GLuint program_from_source(const uint8_t* vsData, const size_t vsSize, const uint8_t* fsData, const size_t fsSize);

private:
	GLFWwindow* _window;
	float _time;
	bool _has_focus;
	bool _curser_disabled;
};


#endif // gl_service_h
