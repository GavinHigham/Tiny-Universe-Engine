BEGIN {
	print "//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE."
	print "#ifndef SHADERS_H"
	print "#define SHADERS_H"
	print "#include <GL/glew.h>\n"

	print "struct shader_info {"
	print "	const GLchar **vs_source;"
	print "	const GLchar **fs_source;"
	print "	const GLchar **attr_names;"
	print "	const GLchar **unif_names;"
	print "	const char **vs_file_path;"
	print "	const char **fs_file_path;"
	print "};"
}
FNR == 1 {
	split(FILENAME, arr, /[\/\.]/)
	name = arr[length(arr)-1]
	ext  = arr[length(arr)]
	programs[name]
	shaders[name, ext]
}
#uniforms and attributes are not handled on a per-shader/per-program basis :/
($1 == "in" && ext == "vs") {attributes[substr($3, 1, length($3)-1)]}
($1 == "uniform") {
	split($3, split_result, "[\[;]"); #Hopefully this will make arrays work.
	unif_name = split_result[1]#substr($3, 1, length($3)-1)
	uniforms[split_result[1]]#substr($3, 1, length($3)-1)]
}
END {
	print "struct shader_prog {"
	print "\tGLuint handle;"
	print "\tunion {"
	print "\t\tstruct {"
	for (attribute in attributes) {
		print "\t\t\tGLint " attribute ";"
		attr_len++
	}
	print "\t\t};"
	print "\t\tGLint attr[" attr_len "];"
	print "\t};"
	print "\tunion {"
	print "\t\tstruct {"
	for (uniform in uniforms) {
		print "\t\t\tGLint " uniform ";"
		unif_len++
	}
	print "\t\t};"
	print "\t\tGLint unif[" unif_len "];"
	print "\t};"
	print "};"
	for (prog in programs)
		if (((prog, "vs") in shaders) && ((prog, "fs") in shaders)) {
			print "extern struct shader_prog "prog"_program;"
			numprogs++
		}
	print "extern struct shader_prog *shader_programs["numprogs"];"
	print "extern struct shader_info *shader_infos["numprogs"];"
	print "\n#endif"
}