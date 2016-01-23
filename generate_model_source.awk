BEGIN {
	print "//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE."
	print "#ifndef MODELS_H"
	print "#define MODELS_H\n"
	print "#include <GL/glew.h>"
	print "#include \"buffer_group.h\"\n"
	color_scale = 1/255.0
	gamma = 2.2
}
FNR == 1 {
	split(FILENAME, arr, /[\/\.]/)
	name = arr[length(arr)-1]
	models[name]

	counts[name"positions"] = 0
	counts[name"normals"] = 0
	counts[name"colors"] = 0
	counts[name"indices"] = 0
	data[name"positions", counts[name"positions"]++] = "\tGLfloat positions[] = {"
	data[name"normals", counts[name"normals"]++] = "\tGLfloat normals[] = {"
	data[name"colors", counts[name"colors"]++] = "\tGLfloat colors[] = {"
	data[name"indices", counts[name"indices"]++] = "\tGLuint indices[] = {"
	reading_vertices = 0
	reading_indices = 0
	just_started = 1
	data[name, "using_normals"] = 0
	data[name, "using_colors"] = 0
}

($1 == "property" && $3 == "nx") {data[name, "using_normals"] = 1}
($1 == "property" && $3 == "red") {data[name, "using_colors"] = 1}
($1 == "element" && $2 == "vertex") {vertex_count = $3 + 0}
($1 == "element" && $2 == "face") {face_count = $3 + 0}
($1 == "end_header") {reading_vertices = 1}
(vertex_count > 0 && reading_vertices) {
	if (!just_started) {
		vertex_count--
		if (vertex_count == 0) {
			reading_indices = 1
			reading_vertices = 0
			just_started = 1
		}
		data[name"positions", counts[name"positions"]++] = "\t\t" $1 ", " $2 ", " $3 ","
		if (data[name, "using_normals"])
			data[name"normals", counts[name"normals"]++] = "\t\t" $4 ", " $5 ", " $6 ","
		if (data[name, "using_colors"])
			data[name"colors", counts[name"colors"]++] = "\t\t" ($7*color_scale)^gamma ", " ($8*color_scale)^gamma ", " ($9*color_scale)^gamma ","
	} else {
		just_started = 0
	}
}
reading_indices {
	if (!just_started) {
		data[name"indices", counts[name"indices"]++] = "\t\t" $2 ", " $3 ", " $4 ","
		if ($1 == 4) {
			data[name"indices", counts[name"indices"] - 1] = data[name"indices", counts[name"indices"] - 1] " " $5 ","
		}
	} else {
		just_started = 0
	}
}
END {
	for (model_name in models) {
		print "int buffer_" model_name "(struct buffer_group bg)"
		print "{"

		for (i = 0; i < counts[model_name"positions"] - 1; i++)
			print data[model_name"positions", i]
		print substr(data[model_name"positions", i], 1, length(data[model_name"positions", i])-1) "\n\t};\n"

		if (data[model_name, "using_normals"]) {
			for (i = 0; i < counts[model_name"normals"] - 1; i++)
				print data[model_name"normals", i]
			print substr(data[model_name"normals", i], 1, length(data[model_name"normals", i])-1) "\n\t};\n"
		}

		if (data[model_name, "using_colors"]) {
			for (i = 0; i < counts[model_name"colors"] - 1; i++)
				print data[model_name"colors", i]
			print substr(data[model_name"colors", i], 1, length(data[model_name"colors", i])-1) "\n\t};\n"
		}

		for (i = 0; i < counts[model_name"indices"] - 1; i++)
			print data[model_name"indices", i]
		print substr(data[model_name"indices", i], 1, length(data[model_name"indices", i])-1) "\n\t};\n"
		
		print "\tglBindBuffer(GL_ARRAY_BUFFER, bg.vbo);"
		print "\tglBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);"
		if (data[model_name, "using_colors"]) {
			print "\tglBindBuffer(GL_ARRAY_BUFFER, bg.cbo);"
			print "\tglBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);"
		}
		if (data[model_name, "using_normals"]) {
			print "\tglBindBuffer(GL_ARRAY_BUFFER, bg.nbo);"
			print "\tglBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);"
		}
		print "\tglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.ibo);"
		print "\tglBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);"
		print "\treturn sizeof(indices)/sizeof(indices[0]);"
		print "}\n"
	}
	print "#endif"
}
