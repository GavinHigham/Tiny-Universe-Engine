BEGIN {
	print "//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE."
	print "#include \"shaders.h\""
	print "#include \"shader_program.h\"\n"
}
FNR == 1 {
	split(FILENAME, arr, /[\/\.]/)
	name = arr[length(arr)-1]
	ext  = arr[length(arr)]
	programs[name]
	shaders[name, ext] = "const char *" name"_"ext"_source[] = {\n"
}
($1 == "in" && ext == "vs") {attributes = attributes "\"" substr($3, 1, length($3)-1) "\", "}
($1 == "uniform" && ext == "vs") {uniforms = uniforms "\"" substr($3, 1, length($3)-1) "\", "}
{
	gsub(/"/, "\\\"")
	shaders[name, ext] = shaders[name, ext] "\"" $0 "\\n\"\n"
}
END {
	for (prog in programs)
		if (((prog, "vs") in shaders) && ((prog, "fs") in shaders)) {
			print shaders[prog, "vs"] "};"
			print shaders[prog, "fs"] "};"
			print "const GLchar *"prog"_attribute_names[] = {"substr(attributes, 1, length(attributes)-2)"};"
			print "const GLchar *"prog"_uniform_names[] = {"substr(uniforms, 1, length(uniforms)-2)"};"
			print "static const int "prog"_attribute_count = sizeof("prog"_attribute_names)/sizeof("prog"_attribute_names[0]);"
			print "static const int "prog"_uniform_count = sizeof("prog"_uniform_names)/sizeof("prog"_uniform_names[0]);"
			print "GLint "prog"_attributes["prog"_attribute_count];"
			print "GLint "prog"_uniforms["prog"_uniform_count];"
			print "struct shader_prog "prog"_program = {"
			print"	.vs_source = "prog"_vs_source,"
			print"	.fs_source = "prog"_fs_source,"
			print"	.handle = 0,"
			print"	.attr_cnt = "prog"_attribute_count,"
			print"	.unif_cnt = "prog"_uniform_count,"
			print"	.attr = "prog"_attributes,"
			print"	.unif = "prog"_uniforms,"
			print"	.attr_names = "prog"_attribute_names,"
			print"	.unif_names = "prog"_uniform_names,"
			print"};"
		}
}