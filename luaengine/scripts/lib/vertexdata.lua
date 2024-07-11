local util,gl = require 'lib/util'.safe()
local ply = require 'models/parse_ply'
local VertexData = {}

function VertexData.PlyFileVertexData(filename)
	local plyfile = assert(ply.parseFile(filename))
	print(filename, 'num elements:', #plyfile)
	local vdata = VertexData.VertexData('vec3 position, vec3 normal, vec3 color')
	for j,element in ipairs(plyfile) do
		print('element', element.name, 'data count', #element.data)
		if element.name == 'vertex' then
			vdata.vertices(element.data)
		elseif element.name == 'face' then
			local indices = {}
			for i,indexList in ipairs(element.data) do
				table.move(indexList, 1, #indexList, #indices+1, indices)
				-- table.insert(indices, prim_restart_idx)
			end
			vdata.indices(indices)
		end
	end
	vdata.mode(gl.TRIANGLES)
	return vdata
end

-- local counts = {float = 1, int = 1, bool = 1, vec2 = 2, vec3 = 3, vec4 = 4}
-- local function componentCount(sortedAttributes)
-- 	local num_components = 0
-- 	for i,v in ipairs(sortedAttributes) do
-- 		num_components = num_components + counts[v.type]
-- 	end
-- 	return num_components
-- end

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
		print('gl.VertexAttribPointer', v.location, vcount, ttype, false, totalsize, offset)
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

--VAOs are stored in a weak table, if no ShaderProgram or VertexData holds a reference to a VAO, it is deleted.
--We can use strings, ShaderPrograms, and VertexDatas as indices for easy lookup - weak keys mean that being used
--as indices does not prevent their collection.
local vertexDataCommon = setmetatable({}, {__mode = 'k'})
function VertexData.getVertexDataCommon(index)
	-- print('Looking up common with index', index)
	local common = vertexDataCommon[index]
	local sortedAttributes, attributesStr
	if not common then
		if type(index.getAttributes) == 'function' then
			sortedAttributes, attributesStr = index.getAttributes()
			if not sortedAttributes or not attributesStr then
				print('No sortedAttributes or attributesStr, returning nil')
				return nil
			end
			print('Looking up common with attributes string', attributesStr)
			common = vertexDataCommon[attributesStr]
			if common then
				print('Found common, saving with index', index)
				vertexDataCommon[index] = common
			end
		elseif type(index) ~= 'string' then
			print('index is not string and has no way to retrieve attributes string')
			print('index', index, 'with type', type(index), 'and .getAttributes', index.getAttributes)
			return nil
		end
	end

	if not common then
		print('Did not find common, creating')
		common = {
			--TODO: Allocate more than 1 VAO at once?
			vao = gl.VertexArrays(1)[1],
			vertexDatas = setmetatable({}, {__mode = 'kv'}),
			buffers = gl.Buffers(3),
		}
		print('Saving common', common, 'with index', index)
		vertexDataCommon[index] = common
		if attributesStr then
			print('Saving common', common, 'with attributes string', attributesStr)
			vertexDataCommon[attributesStr] = common
		end
	end
	return common
end

local function prepareVertexDataCommonForDraw(index)
	local common = assert(VertexData.getVertexDataCommon(index), 'prepareVertexDataCommonForDraw did not find common')
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
	-- local sortedVertexDatas = {}
	-- for vertexData,_ in pairs(common.vertexDatas) do
	-- 	table.insert(sortedVertexDatas, vertexData)
	-- end
	-- table.sort(sortedVertexDatas, function(a,b) return a.vd_id < b.vd_id end)

	for vertexData,_ in pairs(common.vertexDatas) do
	-- for i,vertexData in ipairs(sortedVertexDatas) do
		vertexData.putShared(function(vertices, indices, adjacency)
			print('putting shared')
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
			print('baseVertex:', baseVertex, ', componentCount:', componentCount)
			return baseVertex/componentCount, indicesPtr*4, adjacencyPtr*4
		end)
	end
	gl.BindVertexArray(common.vao)
	--TODO: Calculate proper size
	--maybe, gl.PackGeneric()? Could take something to tell how to interpret vertices.
	assert(#all_vertices > 0, 'No vertices???')
	if #all_vertices > 0 then
		gl.BindBuffer(gl.ARRAY_BUFFER, common.buffers[1])
		gl.BufferData(gl.ARRAY_BUFFER, 4*num_vertices, gl.Pack32f(table.unpack(all_vertices)), gl.STATIC_DRAW)
	end
	if #all_indices > 0 then
		gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, common.buffers[2])
		gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, 4*num_indices, gl.Pack32i(table.unpack(all_indices)), gl.STATIC_DRAW)
	end
	if #all_adjacency > 0 then
		gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, common.buffers[3])
		gl.BufferData(gl.ELEMENT_ARRAY_BUFFER, 4*num_adjacency, gl.Pack32i(table.unpack(all_adjacency)), gl.STATIC_DRAW)
	end
	common.readyToDraw = common.vaoReady
end

local current_vd_id = 0
function VertexData.VertexData(s)
	local attributes = s
	local vertices, indices, adjacency
	local indexCount = 0
	local vertexCount = 0
	local componentCount = 0
	local mode = gl.TRIANGLES
	local readyToDraw = false
	local batch = false
	local baseVertex, indicesPtr, adjacencyPtr
	local instanceCount = 1

	VertexData.getVertexDataCommon(s).readyToDraw = false

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
		print('indexCount:', indexCount)
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
	function vertexData.share(b)
		local common = VertexData.getVertexDataCommon(attributes)
		if common then
			common.vertexDatas[vertexData] = (b and true) or nil
			print('Sharing')
		else
			print('Could not find common')
		end
		return vertexData
	end
	vertexData.share(true)

	function vertexData.putShared(share)
		baseVertex, indicesPtr, adjacencyPtr = share(vertices, indices, adjacency)
		print('baseVertex', baseVertex, 'indicesPtr', indicesPtr, 'adjacencyPtr', adjacencyPtr)
	end

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

	function vertexData.draw(program)
		local programAttributes, programAttributesStr = program.getAttributes()
		assert(attributes == programAttributesStr, 'Buffer attribute list does not match shader program attribute list.'
			..'buffer attribute list: '..(attributes or '(nil)')
			..'\nshader program attribute list: '..(programAttributesStr or '(nil)'), 2)

		local common = assert(VertexData.getVertexDataCommon(program), 'vertexData.draw did not find common')
		if not common.readyToDraw then
			prepareVertexDataCommonForDraw(program)
		end

		assert(common.vao)
		gl.BindVertexArray(common.vao)
		gl.BindBuffer(gl.ARRAY_BUFFER, common.buffers[1])
		gl.BindBuffer(gl.ELEMENT_ARRAY_BUFFER, common.buffers[2])
		gl.UseProgram(program)
		if indexCount > 0 then
			gl.DrawElementsInstancedBaseVertex(mode, indexCount, gl.UNSIGNED_INT, indicesPtr, instanceCount, baseVertex)
		else
			gl.DrawArraysInstanced(mode, baseVertex, vertexCount, 1)
		end
	end

	return vertexData
end

return VertexData