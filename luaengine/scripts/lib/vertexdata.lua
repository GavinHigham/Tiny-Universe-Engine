local util,gl = require 'lib/util'.safe()
local ply = require 'models/parse_ply'
local VertexData = {}

local print_debug = function() end
-- print_debug = print

--Creates a VertexData object from a .ply file
function VertexData.FromPlyFile(vertexDataGroup, filename)
	local plyfile = assert(ply.parseFile(filename))
	print_debug(filename, 'num elements:', #plyfile)
	local vdata = vertexDataGroup('vec3 position, vec3 normal, vec3 color')
	for j,element in ipairs(plyfile) do
		print_debug('element', element.name, 'data count', #element.data)
		if element.name == 'vertex' then
			vdata.vertices(element.data)
		elseif element.name == 'face' then
			local indices = {}
			for i,indexList in ipairs(element.data) do
				table.move(indexList, 1, #indexList, #indices+1, indices)
			end
			vdata.indices(indices)
		end
	end
	vdata.mode(gl.TRIANGLES)
	return vdata
end

--Creates a VertexData object from a .ply file, including only vertex positions.
--mode allows generation of triangle adjacency index buffer
function VertexData.FromPlyFilePosition(vertexDataGroup, filename, mode)
	local plyfile = assert(ply.parseFile(filename, false, mode == 'adjacency'))
	print_debug(filename, 'num elements:', #plyfile)
	local vdata = vertexDataGroup('vec3 position')
	for j,element in ipairs(plyfile) do
		print_debug('element', element.name, 'data count', #element.data)
		if element.name == 'vertex' then
			--This is a little brittle
			--extract the first 3 elements from each vertex
			--element.data has all elements contiguous in a single table
			local vertices = {}
			local vertex_size = #element.properties
			for i = 1, #element.data - vertex_size + 1, vertex_size do
				table.insert(vertices, element.data[i])
				table.insert(vertices, element.data[i+1])
				table.insert(vertices, element.data[i+2])
			end
			vdata.vertices(vertices)
		elseif element.name == 'face' then
			local indices = {}
			for i,indexList in ipairs(element.data) do
				table.move(indexList, 1, #indexList, #indices+1, indices)
				-- table.insert(indices, prim_restart_idx)
			end
			vdata.indices(indices)
		elseif mode == 'adjacency' and element.name == 'triangle_adjacency' then
			vdata.adjacency(element.data)
		end
	end
	if mode == 'adjacency' then
		vdata.mode(gl.TRIANGLES_ADJACENCY)
	else
		vdata.mode(gl.TRIANGLES)
	end
	return vdata
end

function VertexData.PlyFileVertexData(filename)
	return VertexData.FromPlyFile(VertexData.VertexData, filename)
end

-- local counts = {float = 1, int = 1, bool = 1, vec2 = 2, vec3 = 3, vec4 = 4}
-- local function componentCount(sortedAttributes)
-- 	local num_components = 0
-- 	for i,v in ipairs(sortedAttributes) do
-- 		num_components = num_components + counts[v.type]
-- 	end
-- 	return num_components
-- end

--Given a VAO and a table of vertex attributes sorted by location, enables each vertex attribute and sets the attribute pointer.
--Assumes that the attribute data are stored in a packed interleaved buffer.
--The schema for each attribute in the sortedAttributes table is:
--{name = <string>attribute_name, type = <string>attribute_type, count = <Number>count, location = <Number>location}
local function vao_initAttributes(vao, sortedAttributes)
	--TODO: handle GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_INT, and GL_UNSIGNED_INT
	local counts = {float = 1, int = 1, bool = 1, vec2 = 2, vec3 = 3, vec4 = 4}
	local sizes = {float = 4, int = 4, bool = 1 --[[check this one]], vec2 = 8, vec3 = 12, vec4 = 16}
	local types = {float = gl.FLOAT, int = gl.INT, bool = gl.BYTE --[[check this one]], vec2 = gl.FLOAT, vec3 = gl.FLOAT, vec4 = gl.FLOAT}

	gl.BindVertexArray(vao)
	--{name = attribute_name, type = attribute_type, count = count, location = attribute_location}
	local totalsize = 0
	local num_components = 0
	local offset = 0
	for i,v in ipairs(sortedAttributes) do
		totalsize = totalsize + sizes[v.type]
	end
	for i,v in ipairs(sortedAttributes) do
		gl.EnableVertexAttribArray(v.location)
		local vtype = v.type
		local ttype = types[vtype]
		local vcount = counts[vtype]
		if ttype == nil then error('The vertex type "'..(vtype or 'nil')..'" has not been implemented yet!') end
		print_debug('gl.VertexAttribPointer', v.location, vcount, ttype, false, totalsize, offset)
		if vtype == gl.INT or vtype == gl.BYTE then
			gl.VertexAttribPointer(v.location, vcount, ttype, totalsize, offset)
		else
			gl.VertexAttribPointer(v.location, vcount, ttype, false, totalsize, offset)
		end
		num_components = num_components + vcount
		offset = offset + sizes[vtype]
	end
	return num_components
end

--Within a given vertexDataGroup, each unique vertex attribute combination shares a "common" vertex buffer.
--index may be a string representation of the attributes, or an object containing a getAttributes function (such as a ShaderProgram)
local function getVertexDataCommonBuffer(vertexDataGroup, index)
	-- print_debug('Looking up common with index', index)
	local common = vertexDataGroup[index]
	local sortedAttributes, attributesStr
	if not common then
		if type(index.getAttributes) == 'function' then
			sortedAttributes, attributesStr = index.getAttributes()
			if not sortedAttributes or not attributesStr then
				print_debug('No sortedAttributes or attributesStr, returning nil')
				return nil
			end
			print_debug('Looking up common with attributes string', attributesStr)
			common = vertexDataGroup[attributesStr]
			if common then
				print_debug('Found common, saving with index', index)
				vertexDataGroup[index] = common
			end
		elseif type(index) ~= 'string' then
			print_debug('index is not string and has no way to retrieve attributes string')
			print_debug('index', index, 'with type', type(index), 'and .getAttributes', index.getAttributes)
			return nil
		end
	end

	if not common then
		print_debug('Did not find common, creating')
		common = {
			--TODO: Allocate more than 1 VAO at once?
			vao = gl.VertexArrays(1)[1],
			vertexDatas = setmetatable({}, {__mode = 'kv'}),
			buffers = gl.Buffers(3),
		}
		print_debug('Saving common', common, 'with index', index)
		vertexDataGroup[index] = common
		if attributesStr then
			print_debug('Saving common', common, 'with attributes string', attributesStr)
			vertexDataGroup[attributesStr] = common
		end
	end
	return common
end

local function prepareVertexDataCommonBufferForDraw(vertexDataGroup, index)
	local common = assert(getVertexDataCommonBuffer(vertexDataGroup, index), 'prepareVertexDataCommonForDraw did not find common')
	local componentCount

	if type(index.getAttributes) == 'function' then
		local sortedAttributes, attributesStr = index.getAttributes()
		--glVertexAttribPointer wants a bound buffer if "pointer" is nonzero
		gl.BindBuffer(gl.ARRAY_BUFFER, common.buffers[1])
		componentCount = vao_initAttributes(common.vao, sortedAttributes)
		common.componentCount = componentCount
		for vertexData in pairs(common.vertexDatas) do
			vertexData.componentCount(componentCount)
		end
		common.vaoReady = true
	end

	local all_vertices, all_indices, all_adjacency = {}, {}, {}
	local num_vertices, num_indices, num_adjacency = 0, 0, 0
	local int32_size, float32_size = 4, 4
	-- local sortedVertexDatas = {}
	-- for vertexData,_ in pairs(common.vertexDatas) do
	-- 	table.insert(sortedVertexDatas, vertexData)
	-- end
	-- table.sort(sortedVertexDatas, function(a,b) return a.vd_id < b.vd_id end)

	for vertexData,putShared in pairs(common.vertexDatas) do
		--[[See the comment on vertexData putShared,
		essentially just a way to get the internal data and then update the internal pointers,
		we give the vertexData's putShared closure our own closure,
		which it calls to provide us with the internal data and then update its internal pointer variables

		Keeping these encapsulated because they're the core tricky part that this whole class is trying to safely manage]]
		putShared(function(vertices, indices, adjacency)
			print_debug('putting shared')
			local baseVertex = num_vertices
			local indicesPtr = num_indices
			local adjacencyPtr = num_adjacency
			if vertices then
				table.insert(all_vertices, vertices)
				num_vertices = num_vertices + #vertices
			end
			if indices then
				table.insert(all_indices, indices)
				num_indices = num_indices + #indices
			end
			if adjacency then
				table.insert(all_adjacency, adjacency)
				num_adjacency = num_adjacency + #adjacency
			end
			print_debug('baseVertex:', baseVertex, ', componentCount:', componentCount)
			return baseVertex/componentCount, indicesPtr*int32_size, adjacencyPtr*int32_size
		end)
	end
	gl.BindVertexArray(common.vao)
	--TODO: Calculate proper size
	--maybe, gl.PackGeneric()? Could take something to tell how to interpret vertices.
	assert(#all_vertices > 0, 'No vertices???')
	if #all_vertices > 0 then
		gl.BindBuffer(gl.ARRAY_BUFFER, common.buffers[1])
		gl.BufferData(gl.ARRAY_BUFFER, int32_size*num_vertices, gl.Pack32f(table.unpack(all_vertices)), gl.STATIC_DRAW)
	end
	if #all_indices > 0 then
		gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, common.buffers[2])
		gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, int32_size*num_indices, gl.Pack32i(table.unpack(all_indices)), gl.STATIC_DRAW)
	end
	if #all_adjacency > 0 then
		gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, common.buffers[3])
		gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, int32_size*num_adjacency, gl.Pack32i(table.unpack(all_adjacency)), gl.STATIC_DRAW)
	end
	common.readyToDraw = common.vaoReady
end

local current_vd_id = 0
local function _VertexData(vertexDataGroup, s)
	local attributes = s
	local vertices, indices, adjacency
	local indexCount = 0
	local vertexCount = 0
	local componentCount = 0
	local mode = gl.TRIANGLES
	--Delete these
	local batch = false
	local readyToDraw = false
	--Delete those
	local baseVertex, indicesPtr, adjacencyPtr
	local instanceCount = 1

	--Set the dirty flag on the common buffer
	getVertexDataCommonBuffer(vertexDataGroup, s).readyToDraw = false

	--TODO: Dirty slices for buffers (would only buffer the changed part of the buffer)

	local vertexData = {vd_id = current_vd_id}
	current_vd_id = current_vd_id + 1
	function vertexData.attributes(s)
		attributes = s
		return vertexData
	end
	function vertexData.vertices(t)
		vertices = t
		return vertexData
	end
	function vertexData.indices(t)
		indices = t
		indexCount = #indices
		print_debug('indexCount:', indexCount)
		return vertexData
	end
	function vertexData.adjacency(t)
		adjacency = t
		return vertexData
	end
	function vertexData.mode(m)
		mode = m
		return vertexData
	end
	function vertexData.instance(c)
		instanceCount = c
		return vertexData
	end

	--[[
	places the vertices, indices and ajacency into a common buffer shared with other VertexData

	prepareVertexDataCommonBufferForDraw:
		1. loops through all the VertexData that share a common buffer,
		2. calls each's putShared closure, passing its own closure as an argument

	the internal closure:
		1. takes the vertices, indices and adjacency as an argument,
		2. computes and returns the updated baseVertex, indicesPtr and adjacencyPtr

	This is convoluted but can be thought of as 
	]]
	local function putShared(share)
		baseVertex, indicesPtr, adjacencyPtr = share(vertices, indices, adjacency)
		print_debug('baseVertex', baseVertex, 'indicesPtr', indicesPtr, 'adjacencyPtr', adjacencyPtr)
	end

	function vertexData.share(b)
		local common = getVertexDataCommonBuffer(vertexDataGroup, attributes)
		if common then
			--Add or remove self from the list of vertexDatas sharing the common buffer
			common.vertexDatas[vertexData] = (b and putShared) or nil
			print_debug('Sharing')
		else
			print_debug('Could not find common')
		end
		return vertexData
	end
	vertexData.share(true)

	function vertexData.componentCount(c)
		componentCount = c
		vertexCount = #vertices / componentCount
	end

	-- function vertexData.prepareForDraw(program)
	-- 	local programAttributes, programAttributesStr = program.getAttributes()
	-- 	if attributes ~= programAttributesStr then
	-- 		error('Buffer attribute list does not match shader program attribute list.'
	-- 			..'buffer attribute list: '..(attributes or '(nil)')
	-- 			..'\nshader program attribute list: '..(programAttributesStr or '(nil)'), 2)
	-- 	end

	-- 	local numBuffers = (vertices and 1 or 0) + (indices and 1 or 0) + (adjacency and 1 or 0)
	-- 	local buffers = gl.Buffers(numBuffers)
	-- 	for i = 1, numBuffers do
	-- 		if vertices and not verticesBuffer then
	-- 			verticesBuffer = buffers[i]
	-- 		elseif indices and not indicesBuffer then
	-- 			indicesBuffer = buffers[i]
	-- 		elseif adjacency and not adjacencyBuffer then
	-- 			adjacencyBuffer = buffers[i]
	-- 		end
	-- 	end

	-- 	if vertices then
	-- 		gl.BindBuffer(gl.ARRAY_BUFFER, verticesBuffer)
	-- 		gl.BufferData(gl.ARRAY_BUFFER, 4*#vertices, gl.Pack32f(vertices), gl.STATIC_DRAW)
	-- 	end
	-- 	if indices then
	-- 		gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, indicesBuffer)
	-- 		--TODO: Calculate proper size
	-- 		gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, 4*#indices, gl.Pack32i(indices), gl.STATIC_DRAW)
	-- 	end
	-- 	if adjacency then
	-- 		gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, adjacencyBuffer)
	-- 		--TODO: Calculate proper size
	-- 		gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, 4*#adjacency, gl.Pack32i(adjacency), gl.STATIC_DRAW)
	-- 	end

	-- 	--TODO: track vertex array dirty state, only set attrib pointers when needed
	-- 	vao = VertexData.getVertexArray(program)
	-- 	componentCount = vao_initAttributes(vao, programAttributes)
	-- 	vertexCount = #vertices / componentCount
	-- 	readyToDraw = true
	-- end

	local adjacencyModes = {
		[gl.TRIANGLE_STRIP_ADJACENCY] = true,
		[gl.TRIANGLES_ADJACENCY] = true,
		[gl.LINE_STRIP_ADJACENCY] = true,
		[gl.LINES_ADJACENCY] = true,
		[gl.TRIANGLE_STRIP_ADJACENCY] = true
	}

	function vertexData.draw(program)
		local programAttributes, programAttributesStr = program.getAttributes()
		if attributes ~= programAttributesStr then
			error('Buffer attribute list does not match shader program attribute list.'
			..'\nbuffer attribute list: '..(attributes or '(nil)')
			..'\nshader program attribute list: '..(programAttributesStr or '(nil)'), 2)
		end

		local common = assert(getVertexDataCommonBuffer(vertexDataGroup, program), 'vertexData.draw did not find common')
		if not common.readyToDraw then
			prepareVertexDataCommonBufferForDraw(vertexDataGroup, program)
		end

		assert(common.vao)
		local isAdjacencyMode = adjacencyModes[mode]
		gl.BindVertexArray(common.vao)
		gl.BindBuffer(gl.ARRAY_BUFFER, common.buffers[1])
		if isAdjacencyMode then
			gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, common.buffers[3])
		else
			gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, common.buffers[2])
		end
		gl.UseProgram(program)

		if isAdjacencyMode then
			if indexCount > 0 then
				gl.DrawElementsInstancedBaseVertex(mode, indexCount*2, gl.UNSIGNED_INT, adjacencyPtr, instanceCount, baseVertex)
			else
				gl.DrawArraysInstanced(mode, baseVertex, vertexCount, 1)
			end
		else
			if indexCount > 0 then
				gl.DrawElementsInstancedBaseVertex(mode, indexCount, gl.UNSIGNED_INT, indicesPtr, instanceCount, baseVertex)
			else
				gl.DrawArraysInstanced(mode, baseVertex, vertexCount, 1)
			end
		end
	end

	return vertexData
end

--Returns a "VertexDataGroup". VertexDatas with the same schema (matching attributes) are stored in the same buffer,
--within a given group. When a program is used to draw a VertexData, the attributes expected by the program are checked
--against the attributes of the VertexData to ensure compatability.
function VertexData.VertexDataGroup()
	--VAOs are stored in a weak table, if no ShaderProgram or VertexData holds a reference to a VAO, it is deleted.
	--We can use strings, ShaderPrograms, and VertexDatas as indices for easy lookup - weak keys mean that being used
	--as indices does not prevent their collection.
	return setmetatable({}, {__mode = 'k', __call = _VertexData})
end

--Create a "default" VertexDataGroup
VertexData.VertexData = VertexData.VertexDataGroup()

return VertexData