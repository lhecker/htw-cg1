{
	'target_defaults': {
		'configurations': {
			'Debug': {
				'xcode_settings': {
					'COPY_PHASE_STRIP': 'NO',
				},
			},
			'Release': {
				'xcode_settings': {
					'COPY_PHASE_STRIP': 'YES',
				},
			},
		},
		'msvs_settings': {
			'VCLinkerTool': {
				'SubSystem': '2',
			},
		},
	},

	'targets': [
		{
			'target_name': 'glcraft',
			'type': 'executable',
			'dependencies': [
				'deps/glew.gyp:libglew',
				'deps/glfw.gyp:libglfw',
			],
			'include_dirs': [
				'deps',
				'deps/glm',
			],
			'sources': [
				'assets/shaders/cube.fs',
				'assets/shaders/cube.vs',
				'assets/shaders/white.fs',
				'assets/shaders/white.vs',
				'assets/textures/textures.png',
				'deps/lodepng/picopng.cc',
				'deps/lodepng/picopng.h',
				'src/gl_service.cc',
				'src/gl_service.h',
				'src/main.cc',
			],
			'conditions': [
				['OS=="mac"', {
					'sources': [
						'src/Info.plist',
					],
					'mac_bundle': 1,
					'copies': [
						{
							'destination': '<(PRODUCT_DIR)/glcraft.app/Contents/Resources/assets/shaders',
							'files': [
								'assets/shaders/cube.fs',
								'assets/shaders/cube.vs',
								'assets/shaders/white.fs',
								'assets/shaders/white.vs',
							],
						}, {
							'destination': '<(PRODUCT_DIR)/glcraft.app/Contents/Resources/assets/textures',
							'files': [
								'assets/textures/textures.png',
							],
						},
					],
					'xcode_settings': {
						'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
						'CLANG_CXX_LIBRARY': 'libc++',
						'CLANG_ENABLE_OBJC_ARC': 'YES',
						'INFOPLIST_FILE': '$(PROJECT_DIR)/src/Info.plist',
						'MACOSX_DEPLOYMENT_TARGET': '10.9',
					},
					'link_settings': {
						'libraries': [
							'$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
							'$(SDKROOT)/System/Library/Frameworks/Quartz.framework',
							'$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
						],
					},
				}],
			],
		},
	],
}
