GLint deferred_attributes[attribute_count];
GLint deferred_uniforms[uniform_count];

GLint deferred_shader_attributes(int index)
{
	switch(index)
	{
		
	}
}

int init_deferred_shader()
{
	const GLchar *attribute_names[] = {"vNormal", "vColor", "vPos"};
	const GLchar *uniform_names[] = {"projection_matrix", "NMVM", "MVM"};
	static const int attribute_count = sizeof(attribute_names)/sizeof(attribute_names[0]);
	static const int uniform_count = sizeof(uniform_names)/sizeof(uniform_names[0]);
	GLint attributes[attribute_count];
	GLint uniforms[uniform_count];
	struct shader_prog program = {
		.handle = 0,
		.attr_cnt = attribute_count,
		.unif_cnt = uniform_count,
		.attr = attributes,
		.unif = uniforms,
	};
	struct shader_info info = {
		.vs_source = vs_source,
		.fs_source = fs_source,
		.attr_names = attribute_names,
		.unif_names = uniform_names,
	}

	if (!init_shader_program(&program, info)) {
		printf("Could not compile shader program.\n");
		return -1;
	}
	if (!init_shader_attributes(&program, info)) {
		printf("Could not retrieve shader attributes.\n");
		return -1;
	}
	if (!init_shader_uniforms(&program, info)) {
		printf("Could not retrieve shader uniforms.\n");
		return -1;
	}
}