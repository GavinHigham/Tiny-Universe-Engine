BEGIN {print "#ifndef SHADERS_H\n#define SHADERS_H\n#include \"shader_program.h\"\n"}
FNR == 1 {
	split(FILENAME, arr, /[\/\.]/)
	name = arr[length(arr)-1]
	ext  = arr[length(arr)]
	programs[name] = name
	shaders[name, ext] = name
}
$1 == "in" {attributes = attributes substr($3, 1, length($3)-1) ", "}
END {
	for (prog in programs)
		if (((prog, "vs") in shaders) && ((prog, "fs") in shaders)) {
			print "extern struct shader_prog "prog"_program;"
			print "enum "prog"_attr {"substr(attributes, 1, length(attributes)-2)"};"
		}
	print "\n#endif"
}