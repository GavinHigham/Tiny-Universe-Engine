BEGIN {
	print "//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE."
	print "#ifndef MODELS_H"
	print "#define MODELS_H\n"

	positions = "\tGLfloat positions[] = {"
	normals = "\tGLfloat normals[] = {"
	colors = "\tGLfloat colors[] = {"
	indices = "\tGLuint indices[] = {"
	reading_vertices = 0
	reading_indices = 0
	just_started = 1
	color_scale = 1/255.0
}
FNR == 1 {
	split(FILENAME, arr, /[\/\.]/)
	name = arr[length(arr)-1]
}

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
		positions = positions "\n\t\t" $1 ", " $2 ", " $3 ","
		normals = normals "\n\t\t" $4 ", " $5 ", " $6 ","
		colors = colors "\n\t\t" $7*color_scale ", " $8*color_scale ", " $9*color_scale ","
	} else {
		just_started = 0
	}
}
reading_indices {
	if (!just_started) {
		indices = indices "\n\t\t" $2 ", " $3 ", " $4 ","
		if ($1 == 4) {
			indices = indices " " $5 ","
		}
	} else {
		just_started = 0
	}
}
END {
	print "int buffer_" name "(struct render_context rc)"
	print "{"
	print substr(positions, 1, length(positions)-1) "\n\t};\n"
	print substr(normals, 1, length(normals)-1) "\n\t};\n"
	print substr(colors, 1, length(colors)-1) "\n\t};\n"
	print substr(indices, 1, length(indices)-1) "\n\t};\n"
	print "\tglBindBuffer(GL_ARRAY_BUFFER, rc.vbo);"
	print "\tglBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);"
	print "\tglBindBuffer(GL_ARRAY_BUFFER, rc.cbo);"
	print "\tglBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);"
	print "\tglBindBuffer(GL_ARRAY_BUFFER, rc.nbo);"
	print "\tglBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);"
	print "\tglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rc.ibo);"
	print "\tglBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);"
	print "\treturn sizeof(indices)/sizeof(indices[0]);"
	print "}\n"
	print "#endif"
}
