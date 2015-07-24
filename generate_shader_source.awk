BEGIN {print "#include \"shaders.h\"\n"}
FNR == 1 {
	if (NR > 1)
		print "};"
	split(FILENAME, arr, /[\/\.]/)
	print "const char *" arr[length(arr)-1]"_"arr[length(arr)]"_source[] = {"
}
{print "\"" $0 "\\n\""}
END {print "};"}