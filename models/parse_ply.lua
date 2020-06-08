local ParsePLY = {}

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
		while element.num_read < element.count and tokens do
			local tokens_read = 0 --How many tokens on this line have been consumed so far
			for i,property in ipairs(element.properties) do
				if element.properties[i].type == 'list' then
					local list = {}
					--Append all list elements, excluding the count, to the list table above 
					table.move(tokens, tokens_read + 2, tokens_read + 1 + tonumber(tokens[tokens_read + 1]), 1, list)
					tokens_read = tokens_read + 1 + #list
					print(#list)
					data[#data + 1] = list
				else
					tokens_read = tokens_read + 1
					data[#data + 1] = tokens[tokens_read]
				end
			end

			element.data = data
			element.num_read = element.num_read + 1
			print('Tokens read: '..tokens_read..', '..element.num_read..' elements read.')
			tokens = coroutine.yield()
		end
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
			}
		}
		--... possibly more elements
	}
}
]]
function ParsePLY.parseFile(file)
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
		return parse() or file..': File ended unexpectedly, parse unsuccessful.'
	end
end

--If the provided elements include "vertex" with x,y,z, and "face" describing triangles,
--returns a table of indices with adjacency information (for stencil-buffer shadows, etc.)
function ParsePLY.generateAdjacencies(elements)
	local vertex,face
	for i,element in ipairs(elements) do
		if element.name == 'vertex' then
			vertex = element
		elseif element.name == 'face' then
			face = element
		end
	end

	local ix,iy,iz
	for i,property in ipairs(vertex.properties) do
		if property.name == 'x' then
			ix = i
		elseif property.name == 'y' then
			iy = i
		elseif property.name == 'z' then
			iz = i
		end
	end

	local vertex_size = #vertex.properties
	local function edgeString(data, iv1, iv2)
		local iv1offset = iv1 * vertex_size
		local iv2offset = iv2 * vertex_size
		local str = data[iv1offset + ix]..','..data[iv1offset + iy]..','..data[iv1offset + iz]..'-'..data[iv2offset + ix]..','..data[iv2offset + iy]..','..data[iv2offset + iz]
		print(str)
		return str
	end

	--Create half-edge data structure
	local vertex_data = vertex.data
	local face_data = face.data
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

return ParsePLY
