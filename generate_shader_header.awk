BEGIN {
	print "//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE."
	print "#ifndef SHADERS_H"
	print "#define SHADERS_H"
	print "#include \"shader_program.h\"\n"
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
($1 == "uniform") {uniforms[substr($3, 1, length($3)-1)]}
END {
	for (attribute in attributes) {
		attr_string = attr_string attribute ", "
	}
	for (uniform in uniforms) {
		unif_string = unif_string uniform ", "
	}
	for (prog in programs)
		if (((prog, "vs") in shaders) && ((prog, "fs") in shaders)) {
			print "extern struct shader_prog "prog"_program;"
			if (length(attr_string) > 0) {
				print "enum "prog"_attr {"substr(attr_string, 1, length(attr_string)-2)"};"
			}
			if (length(unif_string) > 0) {
				print "enum "prog"_unif {"substr(unif_string, 1, length(unif_string)-2)"};"
			}
		}
	print "\n#endif"
}