{
	'targets': [
		{
			'target_name': 'libglew',
			'type': 'static_library',
			'include_dirs': [
				'glew/include',
			],
			'defines': [
				'GLEW_NO_GLU',
				'GLEW_STATIC',
			],
			'sources': [
				'glew/src/glew.c',
			],
			'direct_dependent_settings': {
				'include_dirs': [
					'glew/include',
				],
				'defines': [
					'GLEW_NO_GLU',
					'GLEW_STATIC',
				],
			},
		},

		{
			'target_name': 'libglewmx',
			'type': 'static_library',
			'include_dirs': [
				'glew/include',
			],
			'defines': [
				'GLEW_MX',
				'GLEW_NO_GLU',
				'GLEW_STATIC',
			],
			'sources': [
				'glew/src/glew.c',
			],
			'conditions': [
				['OS=="mac"', {
					'link_settings': {
						'libraries': [
							'$(SDKROOT)/System/Library/Frameworks/OpenGL.framework',
						],
					},
				}],
			],
		},
	],
}
