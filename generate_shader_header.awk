BEGIN {print "#ifndef SHADERS_H\n#define SHADERS_H\n"}
FNR == 1 {
	split(FILENAME, arr, /[\/\.]/)
	print "extern const char *" arr[length(arr)-1]"_"arr[length(arr)]"_source[];"
}
END {print "\n#endif"}