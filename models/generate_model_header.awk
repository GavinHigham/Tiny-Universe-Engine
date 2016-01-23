BEGIN {
	print "//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE."
	print "#ifndef MODELS_H"
	print "#define MODELS_H\n"
	print "#include <GL/glew.h>"
	print "#include \"../buffer_group.h\"\n"
}
FNR == 1 {
	split(FILENAME, arr, /[\/\.]/)
	print "int buffer_" arr[length(arr)-1] "(struct buffer_group rc);"
}
END {print "#endif"}
