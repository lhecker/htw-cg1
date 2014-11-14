{
	'targets': [
		{
			'target_name': 'libglfw',
			'type': 'static_library',
			'include_dirs': [
				'glfw/src',
			],
			'sources': [
				'glfw/src/clipboard.c',
				'glfw/src/context.c',
				'glfw/src/gamma.c',
				'glfw/src/init.c',
				'glfw/src/input.c',
				'glfw/src/joystick.c',
				'glfw/src/monitor.c',
				'glfw/src/time.c',
				'glfw/src/window.c',
			],
			'defines': [
				'_GLFW_USE_OPENGL=1',
				'GLFW_VERSION_MAJOR=3',
				'GLFW_VERSION_MINOR=0',
				'GLFW_VERSION_PATCH=4',
				'GLFW_VERSION_EXTRA=',
				'GLFW_VERSION=3.0',
				'GLFW_VERSION_FULL=3.0.4',
			],
			'conditions': [
				['OS=="linux"', {
					'sources': [
						'glfw/src/glx_context.c',
						'glfw/src/x11_clipboard.c',
						'glfw/src/x11_gamma.c',
						'glfw/src/x11_init.c',
						'glfw/src/x11_joystick.c',
						'glfw/src/x11_monitor.c',
						'glfw/src/x11_time.c',
						'glfw/src/x11_unicode.c',
						'glfw/src/x11_window.c',
					],
					'defines': [
						'_GLFW_GLX=1',
						'_GLFW_X11=1',
					],
					'link_settings': {
						'libraries': [
							'-lX11',
							'-lXrandr',
							'-lXinerama',
							'-lXinput',
							'-lXxf86vm',
							'-lXkb',
							'-lXcursor',
							'-lrt',
							'-lm',
						],
					},
				}],
				['OS=="mac"', {
					'sources': [
						'glfw/src/cocoa_clipboard.m',
						'glfw/src/cocoa_gamma.c',
						'glfw/src/cocoa_init.m',
						'glfw/src/cocoa_joystick.m',
						'glfw/src/cocoa_monitor.m',
						'glfw/src/cocoa_time.c',
						'glfw/src/cocoa_window.m',
						'glfw/src/nsgl_context.m',

					],
					'defines': [
						'_GLFW_COCOA=1',
						'_GLFW_NSGL=1',
						'_GLFW_USE_CHDIR=1',
						'_GLFW_USE_MENUBAR=1',
						'_GLFW_USE_RETINA=1',
					],
					'link_settings': {
						'libraries': [
							'$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
							'$(SDKROOT)/System/Library/Frameworks/OpenGL.framework',
							'$(SDKROOT)/System/Library/Frameworks/IOKit.framework',
							'$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
							'$(SDKROOT)/System/Library/Frameworks/CoreVideo.framework',
						],
					},
				}],
				['OS=="win"', {
					'sources': [
						'glfw/src/wgl_context.c',
						'glfw/src/win32_clipboard.c',
						'glfw/src/win32_gamma.c',
						'glfw/src/win32_init.c',
						'glfw/src/win32_joystick.c',
						'glfw/src/win32_monitor.c',
						'glfw/src/win32_time.c',
						'glfw/src/win32_window.c',
					],
					'defines': [
						'_GLFW_USE_DWM_SWAP_INTERVAL=1',
						'_GLFW_WGL=1',
						'_GLFW_WIN32=1',
						'DWINVER=0x0501',
						'UNICODE',
					],
					'link_settings': {
						'libraries': [
							'-lgdi32',
							'-lkernel32',
							'-lopengl32',
							'-luser32',
						],
					},
				}],
			],
			'direct_dependent_settings': {
				'include_dirs': [
					'glfw/include',
				],
			},
		}
	],
}
