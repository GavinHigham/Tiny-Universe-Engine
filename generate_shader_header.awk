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
($1 == "in" && ext == "vs") {attributes = attributes substr($3, 1, length($3)-1) ", "}
($1 == "uniform" && ext == "vs") {uniforms = uniforms substr($3, 1, length($3)-1) ", "}
END {
	for (prog in programs)
		if (((prog, "vs") in shaders) && ((prog, "fs") in shaders)) {
			print "extern struct shader_prog "prog"_program;"
			if (length(attributes) > 0) {
				print "enum "prog"_attr {"substr(attributes, 1, length(attributes)-2)"};"
			}
			if (length(uniforms) > 0) {
				print "enum "prog"_unif {"substr(uniforms, 1, length(uniforms)-2)"};"
			}
		}
	print "\n#endif"
}