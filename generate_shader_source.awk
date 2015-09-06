BEGIN {
	print "//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE."
	print "#include <GL/glew.h>"
	print "#include \"shaders.h\""
}
FNR == 1 {
	split(FILENAME, arr, /[\/\.]/)
	name = arr[length(arr)-1]
	ext  = arr[length(arr)]
	programs[name]
	shaders[name, ext] = "const char *" name"_"ext"_source[] = {\n"
}
#uniforms and attributes are not handled on a per-shader/per-program basis :/
(($1 == "in" || $5 == "in") && ext == "vs") {
	if ($1 == "in")
		attr_name = substr($3, 1, length($3)-1)
	else if ($5 == "in")
		attr_name = substr($7, 1, length($7)-1)
	attributes[attr_name]
	attributes_by_prog[name, attr_name]
}
($1 == "uniform") {
	unif_name = substr($3, 1, length($3)-1)
	uniforms[unif_name]
	uniforms_by_prog[name, unif_name]
}
{
	gsub(/"/, "\\\"")
	shaders[name, ext] = shaders[name, ext] "\"" $0 "\\n\"\n"
}
END {
	for (attribute in attributes) {
		attr_string = attr_string "\"" attribute "\", "
		attr_len++
	}
	for (uniform in uniforms) {
		unif_string = unif_string "\"" uniform "\", "
		unif_len++
	}
	for (prog in programs) {
		if (((prog, "vs") in shaders) && ((prog, "fs") in shaders)) {
			for (attribute in attributes) {
				if ((prog, attribute) in attributes_by_prog)
					present_attributes = present_attributes "0, "
				else
					present_attributes = present_attributes "-1, "
			}
			for (uniform in uniforms) {
				if ((prog, uniform) in uniforms_by_prog)
					present_uniforms = present_uniforms "0, "
				else
					present_uniforms = present_uniforms "-1, "
			}
			print shaders[prog, "vs"] "};"
			print shaders[prog, "fs"] "};"
			print "const GLchar *"prog"_attribute_names[] = {"substr(attr_string, 1, length(attr_string)-2)"};"
			print "const GLchar *"prog"_uniform_names[] = {"substr(unif_string, 1, length(unif_string)-2)"};"
			print "static const int "prog"_attribute_count = sizeof("prog"_attribute_names)/sizeof("prog"_attribute_names[0]);"
			print "static const int "prog"_uniform_count = sizeof("prog"_uniform_names)/sizeof("prog"_uniform_names[0]);"
			print "GLint "prog"_attributes["prog"_attribute_count];"
			print "GLint "prog"_uniforms["prog"_uniform_count];"
			print "struct shader_prog "prog"_program = {"
			print "	.handle = 0,"
			print "	.attr = {"substr(present_attributes, 1, length(present_attributes)-2)"},"
			print "	.unif = {"substr(present_uniforms, 1, length(present_uniforms)-2)"}"
			print "};"
			print "struct shader_info "prog"_info = {"
			print "	.vs_source = "prog"_vs_source,"
			print "	.fs_source = "prog"_fs_source,"
			print "	.attr_names = "prog"_attribute_names,"
			print "	.unif_names = "prog"_uniform_names,"
			print "};"
			shader_programs = shader_programs "&"prog"_program, "
			shader_infos = shader_infos "&"prog"_info, "
		}
		present_attributes = ""
		present_uniforms = ""
	}
	print "struct shader_prog *shader_programs[] = {"substr(shader_programs, 1, length(shader_programs)-2)"};"
	print "struct shader_info *shader_infos[] = {"substr(shader_infos, 1, length(shader_infos)-2)"};"
}