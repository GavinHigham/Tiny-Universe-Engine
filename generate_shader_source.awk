BEGIN {print "#include \"shaders.h\"\n#include \"shader_program.h\"\n"}
FNR == 1 {
	if (NR > 1)
		shaders[name, ext] = shaders[name, ext] "};"
	split(FILENAME, arr, /[\/\.]/)
	name = arr[length(arr)-1]
	ext  = arr[length(arr)]
	programs[name] = name
	shaders[name, ext] = ""
	shaders[name, ext] = shaders[name, ext] "const char *" name"_"ext"_source[] = {\n"
}
{
	if ($1 == "in" && ext == "vs")
		attributes = attributes "\"" substr($3, 1, length($3)-1) "\", "
	gsub(/"/, "\\\"")
	shaders[name, ext] = shaders[name, ext] "\"" $0 "\\n\"\n"
}
END {
	shaders[name, ext] = shaders[name, ext] "};\n"
	for (prog in programs)
		if (((prog, "vs") in shaders) && ((prog, "fs") in shaders)) {
			print shaders[prog, "vs"]
			print shaders[prog, "fs"]
			print "const GLchar *"prog"_attribute_names[] = {"substr(attributes, 1, length(attributes)-2)"};"
			print "static const int "prog"_attribute_count = sizeof("prog"_attribute_names)/sizeof("prog"_attribute_names[0]);"
			print "GLint "prog"_attributes["prog"_attribute_count];"
			print "struct shader_prog "prog"_program = {"
			print"	.vs_source = "prog"_vs_source,"
			print"	.fs_source = "prog"_fs_source,"
			print"	.handle = 0,"
			print"	.attr_cnt = "prog"_attribute_count,"
			print"	.attr = "prog"_attributes,"
			print"	.attr_names = "prog"_attribute_names,"
			print"};"
		}
}