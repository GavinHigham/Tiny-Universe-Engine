local ParsePLY = {}
local sizes = {
	char = 1,
	uchar = 1,
	short = 2,
	ushort = 2,
	int = 4,
	uint = 4,
	float = 4,
	double = 8,
}

--Shadow the global print function. Comment this out to get useful debug printing.
local function print(...)
end

function ParsePLY.nameFromPath(file)
	local tokens = {}
	for token in string.gmatch(file, "[^./]+") do --Match anything that's not a slash or a dot.
		table.insert(tokens, token)
	end
	return tokens[#tokens-1] --The second to last one is the pure modelName.
end

function coParseLine(tokens)
	local line = 1
	local elements = {}
	local element
	while tokens[1] ~= 'end_header' do
		if tokens[1] == 'element' then
			element = {
				name = tokens[2],
				count = tonumber(tokens[3]),
				num_read = 0,
				properties = {},
				data = {}
			}
			print('Element '..element.name..', count = '..element.count)
			table.insert(elements, element)
		elseif tokens[1] == 'property' then
			if not element then
				return 'Line '..line..': Property declared without prior element declaration'
			end
			if tokens[2] == 'list' then
				table.insert(element.properties, {type = 'list', count_type = tokens[3], item_type = tokens[4], name = tokens[5]})
			else
				table.insert(element.properties, {type = tokens[2], name = tokens[3]})
			end
		end
		line = line + 1
		tokens = coroutine.yield()
	end
	tokens = coroutine.yield()

	for i,element in ipairs(elements) do
		local data = {}
		local data_size = 0
		while element.num_read < element.count and tokens do
			local tokens_read = 0 --How many tokens on this line have been consumed so far
			for i,property in ipairs(element.properties) do
				if property.type == 'list' then
					local list = {}
					--Append all list elements, excluding the count, to the list table above 
					table.move(tokens, tokens_read + 2, tokens_read + 1 + tonumber(tokens[tokens_read + 1]), 1, list)
					tokens_read = tokens_read + 1 + #list
					data_size = data_size + sizes[property.count_type] + #list * sizes[property.item_type]
					table.insert(data, list)
				else
					tokens_read = tokens_read + 1
					data_size = data_size + sizes[property.type]
					table.insert(data, tokens[tokens_read])
				end
			end

			element.num_read = element.num_read + 1
			print('Tokens read: '..tokens_read..', '..element.num_read..' elements read.')
			tokens = coroutine.yield()
		end
		element.data = data
		element.data_size = data_size
		print(element.name .. ' elements read: ' ..element.num_read)
	end
	return elements
end

--[[
Returns a table with the following structure:
{
	elements = {
		[1] = {
			name = "<element name>",
			count = <element count>,
			properties = {
				[1] = {
					type = <type>,
					name = <property name>,
					--if type == list, the following two are also here:
					item_type = <type of each item in the list>,
					count_type = <type of the list count>
				}
				--... possibly more properties
			},
			data = {
				[1] = <the first property of this element>,
				--... all remaining properties of this element.
				--Any list-type property appears as a table of the list items, not including the size prefix
			},
			data_size = <size in bytes required to hold all data in its original format>
		}
		--... possibly more elements
	}
}
if the PLY is a normal 3D model file with vertex and face elements:
	If withTriangleIndexBuffer == true,
		an index buffer is added as an element with name = "triangle".
	If withTriangleAdjacencyIndexBuffer == true,
		an index buffer with adjacent vertex information is added with name = "triangle_adjacency"
]]
function ParsePLY.parseFile(file, withTriangleIndexBuffer, withTriangleAdjacencyIndexBuffer, generateNormals)
	local parse = coroutine.wrap(coParseLine)
	local result

	for line in io.lines(file) do
		--Tokenize line
		local tokens = {}
		for token in string.gmatch(line, "[^%s]+") do
			table.insert(tokens, token)
		end

		result = parse(tokens)
		if result then
			return result
		end
	end

	--If there's no blank line at the end, we'll need one more poke at the coroutine
	if not result then
		result = parse()
	end

	--Optionally generate a simple index buffer and adjacency index buffer.
	--TODO: Optionally generate normals, colors for visualizing models with some sort of standard shader.
	if result and (withTriangleIndexBuffer or withTriangleAdjacencyIndexBuffer) then
		local postProcessSuccess = ParsePLY.postProcess(result, withTriangleIndexBuffer, withTriangleAdjacencyIndexBuffer, generateNormals)
	end

	for i, element in ipairs(result) do
		element.properties = element.properties or {}
	end

	return result or file..': File ended unexpectedly, parse unsuccessful.'
end

function ParsePLY.postProcess(elements, addTriangleIndexBuffer, addTriangleAdjacencyIndexBuffer, addNormals)
	local vertexElement,faceElement,triangleElement,triangleAdjacencyElement
	local vertexElementIndex
	for i,element in ipairs(elements) do
		if element.name == 'vertex' then
			vertexElement = element
			vertexElementIndex = i
		elseif element.name == 'face' then
			faceElement = element
		elseif element.name == 'triangle' then
			triangleElement = element
		elseif element.name == 'triangle_adjacency' then
			triangleAdjacencyElement = element
		end
	end

	print(vertexElement, faceElement, triangleElement, triangleAdjacencyElement, vertexElementIndex)

	if addTriangleIndexBuffer and faceElement and not triangleElement then
		indexBuffer = ParsePLY.generateTriangleBuffer(faceElement)
		table.insert(elements, {
			name = 'triangle',
			count = math.tointeger(#indexBuffer / 3),
			properties = {
				{
					type = 'uint',
					name = 'a',
				},
				{
					type = 'uint',
					name = 'b',
				},
				{
					type = 'uint',
					name = 'c',
				}
			},
			data = indexBuffer,
			data_size = #indexBuffer * sizes.uint
		})
	end
	if addTriangleAdjacencyIndexBuffer and vertexElement and faceElement and not triangleAdjacencyElement then
		adjacencyBuffer = ParsePLY.generateTriangleAdjacencyBuffer(vertexElement, faceElement)
		table.insert(elements, {
			name = 'triangle_adjacency',
			count = math.tointeger(#adjacencyBuffer / 6),
			properties = {
				{
					type = 'uint',
					name = 'a'
				},
				{
					type = 'uint',
					name = 'b'
				},
				{
					type = 'uint',
					name = 'c'
				},
				{
					type = 'uint',
					name = 'd'
				},
				{
					type = 'uint',
					name = 'e'
				},
				{
					type = 'uint',
					name = 'f'
				}
			},
			data = adjacencyBuffer,
			data_size = #adjacencyBuffer * sizes.uint
		})
	end
	if addNormals and vertexElement and faceElement then
		local vertexNormals = ParsePLY.generateNormals(vertexElement, faceElement)
		if not vertexNormals then
			return false
		end

		local newProperties = {}
		for i, property in ipairs(vertexElement.properties) do
			local newProperty = {}
			for k,v in pairs(property) do
				newProperty[k] = v
			end
			table.insert(newProperties, newProperty)
		end
		table.insert(newProperties, {
			type = 'float',
			name = 'nx'
		})
		table.insert(newProperties, {
			type = 'float',
			name = 'ny'
		})
		table.insert(newProperties, {
			type = 'float',
			name = 'nz'
		})

		local newData = {}
		local vertexData = vertexElement.data
		local vertexSize = #(vertexElement.properties)
		local newVertexSize = vertexSize + 3
		for i = 0, vertexElement.count-1 do
			--Append existing vertex properties
			table.move(vertexData, i*vertexSize+1, (i+1)*vertexSize, i*newVertexSize+1, newData)
			--Append new properties (the normals)
			table.move(vertexNormals, i*3+1, i*3+3, i*newVertexSize+vertexSize+1, newData)
		end

		local newVertexElement = {
			name = vertexElement.name,
			count = vertexElement.count,
			properties = newProperties,
			data = newData,
			data_size = vertexElement.data_size + vertexElement.count*3*sizes.float
		}

		elements[vertexElementIndex] = newVertexElement
	end
end

function ParsePLY.getElementsSize(elements)
	local size = 0
	for i,element in ipairs(elements) do
		size = size + element.data_size
	end
	return size
end

--Takes an element representing geometric faceseturns a table of indices (for easy upload to the GPU)
function ParsePLY.generateTriangleBuffer(faceElement)
	local face_data = faceElement.data

	--TODO: Test this function, make sure the winding is correct.
	local function insertStripAsTris(indices, stripIndices)
		local prevA, prevB = stripIndices[1], stripIndices[2]
		for i = 3, #stripIndices do
			table.insert(indices, prevA)
			table.insert(indices, prevB)
			table.insert(indices, stripIndices[i])
			prevB = prevA
			prevA = stripIndices[i]
		end
	end

	local indices = {}

	for i,face in ipairs(face_data) do
		if #face > 3 then
			insertStripAsTris(indices, face)
		else
			table.insert(indices, face[1])
			table.insert(indices, face[2])
			table.insert(indices, face[3])
		end
	end

	return indices
end

--If the provided elements include "vertex" with x,y,z, and "face" describing triangles,
--returns a table of indices with adjacency information (for stencil-buffer shadows, etc.)
function ParsePLY.generateTriangleAdjacencyBuffer(vertexElement, faceElement)
	local ix,iy,iz
	for i,property in ipairs(vertexElement.properties) do
		if property.name == 'x' then
			ix = i
		elseif property.name == 'y' then
			iy = i
		elseif property.name == 'z' then
			iz = i
		end
	end
	if not (ix and iy and iz) then
		print("Trying to generate adjacencies but vertex position properties (x, y, z) do not exist")
		return
	end

	local vertex_size = #vertexElement.properties
	local function edgeString(data, iv1, iv2)
		local iv1offset = iv1 * vertex_size
		local iv2offset = iv2 * vertex_size
		local str = data[iv1offset + ix]..','..data[iv1offset + iy]..','..data[iv1offset + iz]..'-'..data[iv2offset + ix]..','..data[iv2offset + iy]..','..data[iv2offset + iz]
		-- print(str)
		return str
	end

	--Create half-edge data structure
	local vertex_data = vertexElement.data
	local face_data = faceElement.data
	local half_edges = {}
	for i,face in ipairs(face_data) do
		--Only handles triangles
		half_edges[edgeString(vertex_data, face[1], face[2])] = face[3]
		half_edges[edgeString(vertex_data, face[2], face[3])] = face[1]
		half_edges[edgeString(vertex_data, face[3], face[1])] = face[2]
	end

	--Indices with adjacency information
	local indices = {}

	for i,face in ipairs(face_data) do
		table.insert(indices, face[1])
		table.insert(indices, half_edges[edgeString(vertex_data, face[2], face[1])])
		table.insert(indices, face[2])
		table.insert(indices, half_edges[edgeString(vertex_data, face[3], face[2])])
		table.insert(indices, face[3])
		table.insert(indices, half_edges[edgeString(vertex_data, face[1], face[3])])
	end

	return indices
end

function ParsePLY.generateNormals(vertexElement, faceElement)
	local ix,iy,iz,nx,ny,nz
	for i,property in ipairs(vertexElement.properties) do
		if property.name == 'x' then
			ix = i
		elseif property.name == 'y' then
			iy = i
		elseif property.name == 'z' then
			iz = i
		elseif property.name == 'nx' then
			nx = i
		elseif property.name == 'ny' then
			ny = i
		elseif property.name == 'nz' then
			nz = i
		end
	end

	if (nx or ny or nz) and not (nx and ny and nz) then
		print('Trying to generate normals but one or more (but not all) property appears to already exist (nx, ny or nz)')
		return
	end

	local vertexData = vertexElement.data
	local vertexSize = #(vertexElement.properties)
	local vertexNormals = {}

	local sqrt = math.sqrt
	local acos = math.acos
	local fmod = math.fmod

	for i,face in ipairs(faceElement.data) do
		--Only handles triangles
		--if a face is an N-gon, its face normal is determined by the first 3 vertices
		local v1x = vertexData[face[1]*vertexSize+ix]
		local v1y = vertexData[face[1]*vertexSize+iy]
		local v1z = vertexData[face[1]*vertexSize+iz]

		--[[
		                       v1
		             |         /\         |
		             |        /  \        |
		             |   e1  /    \  e2   |
		             |      /      \      |
		             V     /        \     V
		                 v2----------v3
		]]

		local e1x = vertexData[face[2]*vertexSize+ix] - v1x
		local e1y = vertexData[face[2]*vertexSize+iy] - v1y
		local e1z = vertexData[face[2]*vertexSize+iz] - v1z

		local e2x = vertexData[face[3]*vertexSize+ix] - v1x
		local e2y = vertexData[face[3]*vertexSize+iy] - v1y
		local e2z = vertexData[face[3]*vertexSize+iz] - v1z
		--face normal
		local nx, ny, nz = e1y*e2z-e1z*e2y, e1z*e2x-e1x*e2z, e1x*e2y-e1y*e2x
		--face area, used to weight vertex normals
		local area = sqrt(nx*nx + ny*ny + nz*nz)/2
		--compute angle and area weighted vertex normals
		local numFaceVerts = #face
		for j,curr_idx in ipairs(face) do
			local prev_idx = face[fmod(j-2 + numFaceVerts, numFaceVerts)+1]
			local next_idx = face[fmod(j,numFaceVerts)+1]

			local prevX = vertexData[prev_idx*vertexSize+ix]
			local prevY = vertexData[prev_idx*vertexSize+iy]
			local prevZ = vertexData[prev_idx*vertexSize+iz]

			local nextX = vertexData[next_idx*vertexSize+ix]
			local nextY = vertexData[next_idx*vertexSize+iy]
			local nextZ = vertexData[next_idx*vertexSize+iz]

			local currX = vertexData[curr_idx*vertexSize+ix]
			local currY = vertexData[curr_idx*vertexSize+iy]
			local currZ = vertexData[curr_idx*vertexSize+iz]

			local e1x = prevX - currX
			local e1y = prevY - currY
			local e1z = prevZ - currZ
			local e1_mag = sqrt(e1x*e1x + e1y*e1y + e1z*e1z)

			local e2x = nextX - currX
			local e2y = nextY - currY
			local e2z = nextZ - currZ
			local e2_mag = sqrt(e2x*e2x + e2y*e2y + e2z*e2z)

			local angle = acos((e1x*e2x + e1y*e2y + e1z*e2z)/(e1_mag*e2_mag))
			--accumulate angle and area weighted vertex normals
			vertexNormals[3*curr_idx+1] = (vertexNormals[3*curr_idx+1] or 0) + (nx * angle * area)
			vertexNormals[3*curr_idx+2] = (vertexNormals[3*curr_idx+2] or 0) + (ny * angle * area)
			vertexNormals[3*curr_idx+3] = (vertexNormals[3*curr_idx+3] or 0) + (nz * angle * area)
		end
	end

	for i = 1, #vertexNormals-2, 3 do
		local nx = vertexNormals[i]
		local ny = vertexNormals[i+1]
		local nz = vertexNormals[i+2]
		local inv_mag = 1/sqrt(nx*nx + ny*ny + nz*nz)
		vertexNormals[i] = nx * inv_mag
		vertexNormals[i+1] = ny * inv_mag
		vertexNormals[i+2] = nz * inv_mag
	end

	return vertexNormals
end

function ParsePLY.export(elements, filename)
	local file = io.open(filename, 'w+')
	if not filename then
		print('Could not open file for write: ', filename)
		return
	end

	--Write the header
	file:write([[
ply
format ascii 1.0
]])
	for _,element in ipairs(elements) do
		file:write('element ' .. element.name .. ' ' .. element.count .. '\n')
		for _,property in ipairs(element.properties) do
			if property.type == 'list' then
				file:write('property list ' .. property.count_type .. ' ' .. property.item_type .. ' ' .. property.name .. '\n')
			else
				file:write('property ' .. property.type .. ' ' .. property.name .. '\n')
			end
		end
	end
	file:write('end_header\n')

	--Write the data
	for _,element in ipairs(elements) do
		local propertyCount = #element.properties
		local data = element.data
		for i = 0, element.count-1 do
			for j = 1, propertyCount do
				local datum = data[i*propertyCount+j]

				if type(datum) == 'table' then
					file:write(tostring(#datum) .. ' ')
					file:write(table.concat(datum, ' '))
				else
					file:write(datum)
				end

				if j ~= propertyCount then
					file:write(' ')
				end
			end
			file:write('\n')
		end
	end
	file:close()
end

return ParsePLY
