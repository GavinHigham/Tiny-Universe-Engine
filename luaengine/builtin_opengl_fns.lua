--VERTEX_SHADER_TYPE and SHADER_TYPES defined in lua_opengl.c

local types = {float = true, int = true, bool = true, vec2 = true, vec3 = true, vec4 = true}
function attributesFromShaderStrings(shaderStrings, shaderTypes, program)
	--print('shaderStrings='..type(shaderStrings)..', shaderTypes='..type(shaderTypes)..', program='..type(program))
	--print('shaderStrings[1]='..type(shaderStrings[1])..', shaderTypes[1]='..tostring(shaderTypes[1]))
	local attributes, sortedAttributes = {}, {}
	local name = '[%a_][%w_]*'
	for i,s in ipairs(shaderStrings) do
		if shaderTypes[i] == VERTEX_SHADER_TYPE then
			for attribute_type, attribute_name, count in s:gmatch('%s*in%s+('..name..')%s+('..name..')([%[%d%]]*)') do
				if count then
					count = tonumber(count:match('%[(%d+)%]'))
				end
				count = count or 1
				if not attributes[attribute_name] and types[attribute_type] then
					local attribute = {name = attribute_name, type = attribute_type, count = count}
					attributes[attribute_name] = attribute
					table.insert(sortedAttributes, attribute)
				end
				local arraySuffix = ''
				if count > 1 then arraySuffix = '['..count..']' end
				--print(table.concat({'attribute', attribute_type, attribute_name}, ' ')..arraySuffix..';')
			end
		end
	end

	local gl = require 'OpenGL'
	for i, attribute in ipairs(sortedAttributes) do
		attribute.location = gl.GetAttribLocation(program, attribute.name)
	end
	table.sort(sortedAttributes, function(a,b) return a.location > b.location end)
	local attributeString = {}
	for i, attribute in ipairs(sortedAttributes) do
		local arraySuffix = ''
		if attribute.count > 1 then arraySuffix = '['..attribute.count..']' end
		table.insert(attributeString, attribute.type..' '..attribute.name..arraySuffix)
	end
	attributeString = table.concat(attributeString, ', ')
	--print(attributeString)
	return sortedAttributes, attributeString
end

local function uniformsFromShaderString(shaderStrings, uniforms)
	--print('shaderStrings='..type(shaderStrings)..', uniforms='..type(uniforms))
	--print('shaderStrings[1]='..type(shaderStrings[1]))
	uniforms = uniforms or {}
	--Placeholder enum values for each type
	local name = '[%a_][%w_]*'
	for i,s in ipairs(shaderStrings) do
		for uniform_type, uniform_name, count in s:gmatch('%s*uniform%s+('..name..')%s+('..name..')([%[%d%]]*)') do
			if count then
				count = count:match('%[(%d+)%]')
			end
			uniforms[uniform_name] = {type = uniform_type, count = tonumber(count or 1), enumtype = SHADER_TYPES[uniform_type]}
			--local un = uniforms[uniform_name]; print(string.format('uniforms[%s]: type=%s, count=%s, enumtype=%s', uniform_name, un.type, un.count, un.enumtype))
		end
	end
	return uniforms
end

local function shaderSourceError(shader, errorline, errorstring)
	local currentline = 0
	errorline = errorline or tonumber(errorstring:match('%d+:(%d+)'))
	return string.gsub(shader, '[^\\r\\n]*', function (line)
		currentline = currentline + 1
		local linenumber = string.format('%03d: ', currentline)
		if currentline == errorline then
			return '\\27[31m'..linenumber..line..'\\27[0m'
		else
			return linenumber..line
		end
	end)
end

return attributesFromShaderStrings, uniformsFromShaderString, shaderSourceError
