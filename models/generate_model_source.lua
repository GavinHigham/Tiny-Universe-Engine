function edgeString(v1, v2)
	return v1.x .. v1.y .. v1.z .. v2.x .. v2.y .. v2.z
end

function modelNameFromPath(file)
	local tokens = {}
	for token in string.gmatch(file, "[^./]+") do --Match anything that's not a slash or a dot.
		table.insert(tokens, token)
	end
	return tokens[#tokens-1] --The second to last one is the pure modelName.
end

print([[
//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE.
#ifndef MODELS_H
#define MODELS_H

#include <GL/glew.h>
#include "../buffer_group.h"
]])

for _, file in ipairs(arg) do
	local vertices = {}
	local faces = {}
	local halfEdges = {}
	local properties = {}
	--Read in vertices and faces
	local numverts = 0
	local numfaces = 0
	local reading_header = true
	local modelName = modelNameFromPath(file)
	for line in io.lines(file) do
		local tokens = {}
		for token in string.gmatch(line, "[^%s]+") do
			table.insert(tokens, token)
		end
		if reading_header then
			if tokens[1] == "element" then
				if tokens[2] == "vertex" then
					numverts = tonumber(tokens[3])
				end
				if tokens[2] == "face" then
					numfaces = tonumber(tokens[3])
				end
			end
			if tokens[1] == "property" and tokens[2] ~= "list" then
				table.insert(properties, tokens[3])
			end
			if tokens[1] == "end_header" then
				reading_header = false
			end
		else
			if numverts > 0 then
				local new_vert = {}
				for i, property in ipairs(properties) do
					new_vert[property] = tokens[i]
				end
				table.insert(vertices, new_vert)
				numverts = numverts - 1
			elseif numfaces > 0 then
				local new_face = {tokens[2], tokens[3], tokens[4]} --don't support quads
				table.insert(faces, new_face)
				numfaces = numfaces - 1
			end
		end
	end

	--Create half-edge data structure
	for i, face in ipairs(faces) do
		local v1 = vertices[face[1]+1]
		local v2 = vertices[face[2]+1]
		local v3 = vertices[face[3]+1]
		halfEdges[edgeString(v1, v2)] = face[3]
		halfEdges[edgeString(v2, v3)] = face[1]
		halfEdges[edgeString(v3, v1)] = face[2]
	end

	local usingColors = #vertices > 0 and vertices[1].red and vertices[1].blue and vertices[1].green
	local usingNormals = #vertices > 0 and vertices[1].nx and vertices[1].ny and vertices[1].nz

	print("int buffer_" .. modelName .. "(struct buffer_group bg)\n{")
	print("\tGLfloat positions[] = {")
	for i = 1, #vertices do
		local vertex = vertices[i]
		local lineEnd = ","
		if i == #vertices then lineEnd = "\n\t};\n" end
		print("\t\t" .. vertex.x .. ", " .. vertex.y .. ", " .. vertex.z .. lineEnd)
	end
	if usingNormals then
		print("\tGLfloat normals[] = {")
		for i = 1, #vertices do
			local vertex = vertices[i]
			local lineEnd = ","
			if i == #vertices then lineEnd = "\n\t};\n" end
			print("\t\t" .. vertex.nx .. ", " .. vertex.ny .. ", " .. vertex.nz .. lineEnd)
		end
	end
	if usingColors then
		local colorScale = 1/255.0
		local gamma = 2.2
		print("\tGLfloat colors[] = {")
		for i = 1, #vertices do
			local vertex = vertices[i]
			local lineEnd = ","
			if i == #vertices then lineEnd = "\n\t};\n" end
			print("\t\t" .. (vertex.red*colorScale)^gamma .. ", " .. (vertex.green*colorScale)^gamma .. ", " .. (vertex.blue*colorScale)^gamma .. lineEnd)
		end
	end
	--colors, revisit code to make concise, efficient-er
	print("\tGLuint adjacent_indices[] = {")
	for i = 1, #faces do
		local face = faces[i]
		local v1 = vertices[face[1]+1]
		local v2 = vertices[face[2]+1]
		local v3 = vertices[face[3]+1]
		local lineEnd = ","
		if i == #faces then lineEnd = "\n\t};\n" end
		print("\t\t" .. face[1] .. ", " .. halfEdges[edgeString(v2, v1)] .. ", " .. face[2] .. ", " .. halfEdges[edgeString(v3, v2)] .. ", " .. face[3] .. ", " .. halfEdges[edgeString(v1, v3)] .. lineEnd)
		--print("\t\t" .. face[1] .. ", " .. face[2] .. ", " .. face[3] .. lineEnd)
	end
	print("\tGLuint indices[] = {")
	for i = 1, #faces do
		local face = faces[i]
		local v1 = vertices[face[1]+1]
		local v2 = vertices[face[2]+1]
		local v3 = vertices[face[3]+1]
		local lineEnd = ","
		if i == #faces then lineEnd = "\n\t};\n" end
		--print("\t\t" ..halfEdges[edgeString(v2, v1)] .. ", " .. face[2] .. ", " .. halfEdges[edgeString(v3, v2)] .. ", " .. face[3] .. ", " .. halfEdges[edgeString(v1, v3)] .. ", " .. face[1] .. lineEnd)
		print("\t\t" .. face[1] .. ", " .. face[2] .. ", " .. face[3] .. lineEnd)
	end
	print([[
	int attr_index = -1;
	if ((attr_index = buffer_group_attribute_index("vPos")) != -1) {
		glBindBuffer(GL_ARRAY_BUFFER, bg.buffer_handles[attr_index]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
	}]])
	if usingColors then
		print([[
	if ((attr_index = buffer_group_attribute_index("vColor")) != -1) {
		glBindBuffer(GL_ARRAY_BUFFER, bg.buffer_handles[attr_index]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
	}]])
	end
	if usingNormals then
		print([[
	if ((attr_index = buffer_group_attribute_index("vNormal")) != -1) {
		glBindBuffer(GL_ARRAY_BUFFER, bg.buffer_handles[attr_index]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
	}]])
	end
	print([[
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.aibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(adjacent_indices), adjacent_indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	return sizeof(indices)/sizeof(indices[0]);
}]])
end

print("#endif")