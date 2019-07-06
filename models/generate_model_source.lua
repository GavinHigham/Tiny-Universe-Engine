function modelNameFromPath(file)
	local tokens = {}
	for token in string.gmatch(file, "[^./]+") do --Match anything that's not a slash or a dot.
		table.insert(tokens, token)
	end
	return tokens[#tokens-1] --The second to last one is the pure modelName.
end

function print_buffering_function(modelName, usingPositions, usingNormals, usingColors)
	print("int buffer_" .. modelName .. "(struct buffer_group bg)\n{")
	if usingPositions then print([[
	glBindBuffer(GL_ARRAY_BUFFER, bg.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(]]..modelName..[[_positions), ]]..modelName..[[_positions, GL_STATIC_DRAW);]])
	end
	if usingColors then print([[
	glBindBuffer(GL_ARRAY_BUFFER, bg.cbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(]]..modelName..[[_colors), ]]..modelName..[[_colors, GL_STATIC_DRAW);]])
	end
	if usingNormals then print ([[
	glBindBuffer(GL_ARRAY_BUFFER, bg.nbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(]]..modelName..[[_normals), ]]..modelName..[[_normals, GL_STATIC_DRAW);]])
	end
	print([[
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.aibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(]]..modelName..[[_indices_adjacent), ]]..modelName..[[_indices_adjacent, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bg.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(]]..modelName..[[_indices), ]]..modelName..[[_indices, GL_STATIC_DRAW);
	return sizeof(]]..modelName..[[_indices)/sizeof(]]..modelName..[[_indices[0]);]])
	print("}\n")
end

print([[
//GENERATED FILE, CHANGES WILL BE LOST ON NEXT RUN OF MAKE.
#ifndef MODELS_H
#define MODELS_H

#include "graphics.h"
#include "buffer_group.h"
]])

for _, file in ipairs(arg) do
	local vertices = {}
	local faces = {}
	local halfEdges = {}
	local properties = {}
	local prop_idx = {}
	--Read in vertices and faces
	local numverts = 0
	local numfaces = 0
	local reading_header = true
	local modelName = modelNameFromPath(file)

	local function edgeString(v1, v2)
		return v1[prop_idx["x"]] .. v1[prop_idx["y"]] .. v1[prop_idx["z"]] .. v2[prop_idx["x"]] .. v2[prop_idx["y"]] .. v2[prop_idx["z"]]
	end

	for line in io.lines(file) do
		--Tokenize line
		local tokens = {}
		for token in string.gmatch(line, "[^%s]+") do
			table.insert(tokens, token)
		end

		if reading_header then
			if tokens[1] == "element" then
				if tokens[2] == "vertex" then
					numverts = tonumber(tokens[3])
				elseif tokens[2] == "face" then
					numfaces = tonumber(tokens[3])
				end
			elseif tokens[1] == "property" and tokens[2] ~= "list" then
				table.insert(properties, tokens[3])
				prop_idx[tokens[3]] = #properties
			elseif tokens[1] == "end_header" then
				reading_header = false
			end
		else
			if numverts > 0 then
				table.insert(vertices, tokens)
				numverts = numverts - 1
			elseif numfaces > 0 then
				table.insert(faces, {tokens[2], tokens[3], tokens[4]}) --don't support quads
				if tonumber(tokens[1]) > 4 then
					io.stderr:write("N-Gons not supported, check mesh export settings.\n");
				elseif tonumber(tokens[1]) == 4 then
					io.stderr:write("Quads not suggested, will split arbitrarily, check mesh export settings.\n")
					table.insert(faces, {tokens[3], tokens[4], tokens[5]})
				end
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

	local usingPositions = #vertices > 0 and prop_idx["x"]   and prop_idx["y"]    and prop_idx["z"]
	local usingColors    = #vertices > 0 and prop_idx["red"] and prop_idx["blue"] and prop_idx["green"]
	local usingNormals   = #vertices > 0 and prop_idx["nx"]  and prop_idx["ny"]   and prop_idx["nz"]

	if usingPositions then
		print("GLfloat " .. modelName .. "_positions[] = {")
		for i = 1, #vertices do
			local vertex = vertices[i]
			print("\t" .. vertex[prop_idx["x"]] .. ", " .. vertex[prop_idx["y"]] .. ", " .. vertex[prop_idx["z"]] .. ",")
		end
		print("};\n")
	end
	if usingNormals then
		print("GLfloat " .. modelName .. "_normals[] = {")
		for i = 1, #vertices do
			local vertex = vertices[i]
			print("\t" .. vertex[prop_idx["nx"]] .. ", " .. vertex[prop_idx["ny"]] .. ", " .. vertex[prop_idx["nz"]] .. ",")
		end
		print("};\n")
	end
	if usingColors then
		print("unsigned char " .. modelName .. "_colors[] = {")
		for i = 1, #vertices do
			local vertex = vertices[i]
			print("\t" .. vertex[prop_idx["red"]] .. ", " .. vertex[prop_idx["green"]] .. ", " .. vertex[prop_idx["blue"]] .. ",")
		end
		print("};\n")
	end
	--colors, revisit code to make concise, efficient-er
	print("GLuint " .. modelName .. "_indices_adjacent[] = {")
	for i = 1, #faces do
		local face = faces[i]
		local v1 = vertices[face[1]+1]
		local v2 = vertices[face[2]+1]
		local v3 = vertices[face[3]+1]
		print("\t" .. face[1] .. ", " .. halfEdges[edgeString(v2, v1)] .. ", " .. face[2] .. ", " .. halfEdges[edgeString(v3, v2)] .. ", " .. face[3] .. ", " .. halfEdges[edgeString(v1, v3)] .. ",")
		--print("\t\t" .. face[1] .. ", " .. face[2] .. ", " .. face[3] .. ",")
	end
	print("};\n")
	print("GLuint " .. modelName .. "_indices[] = {")
	for i = 1, #faces do
		local face = faces[i]
		local v1 = vertices[face[1]+1]
		local v2 = vertices[face[2]+1]
		local v3 = vertices[face[3]+1]
		--print("\t\t" ..halfEdges[edgeString(v2, v1)] .. ", " .. face[2] .. ", " .. halfEdges[edgeString(v3, v2)] .. ", " .. face[3] .. ", " .. halfEdges[edgeString(v1, v3)] .. ", " .. face[1] .. ",")
		print("\t" .. face[1] .. ", " .. face[2] .. ", " .. face[3] .. ",")
	end
	print("};\n")

	print_buffering_function(modelName, usingPositions, usingNormals, usingColors)
end

print("#endif")