--[[
Uniform class encapsulates a uniform variable, its default value, min, and max,
provides methods to:
	get the uniform handle from the shader
	get updated values from a slider
	update a shader with the new value
]]

function snakeCaseToTitleCase(s)
	return string.gsub(string.gsub(s, "_", " "), "(%w)(%w*)", function(car, cdr) return string.upper(car)..cdr end)
end

function shaderHandleFromShaders(shaders) 
	if #shaders > 0 then
		print(string.match(shaders[1], ".-\n")) --First line
	end
	return #shaders
end
function shaderUniformHandleFromName(name)
	print(name)
	return #name
end
function shaderAttributeHandleFromName(name)
	print(name)
	return #name
end

Uniform = {}

function Uniform:setHandle(shaderPipeline)
	self.handle = shaderPipeline:getUniformHandle(self.name)
end

function Uniform:send(shaderPipeline, value)

end

function Uniform:Class(className)
	class = {}
	mt = {__index = class}
	setmetatable(class, {__index = self})
	self[className] = function(class, defaults)
		for k,v in pairs(defaults) do
			if k ~= "min" and k ~= "max" and k ~= "value" and k ~= "type" then
				if type(k) == "number" then
					defaults.name = defaults.name or v
				else
					defaults.name = defaults.name or k
					defaults.value = defaults.value or v
				end
				break
			end
		end
		defaults.type = className
		if defaults.name and not defaults.display then
			defaults.display = snakeCaseToTitleCase(defaults.name)
		end
		setmetatable(defaults, mt)
		return defaults
	end
end

Uniform:Class "float"
Uniform:Class "vec3"
Uniform:Class "vec4"
Uniform:Class "int"
Uniform:Class "mat4"

Attribute = {}

function Attribute:setHandle(shaderPipeline)
	self.handle = shaderPipeline:getAttributeHandle(self.name)
end

function Attribute:Class(className)
	class = {}
	mt = {__index = class}
	setmetatable(class, {__index = self})
	self[className] = function(class, defaults)
		for k,v in pairs(defaults) do
			if k ~= "type" then
				if type(k) == "number" then
					defaults.name = defaults.name or v
				else
					defaults.name = defaults.name or k
					defaults.value = defaults.value or v
				end
				break
			end
		end
		defaults.type = className
		if defaults.name and not defaults.display then
			defaults.display = snakeCaseToTitleCase(defaults.name)
		end
		setmetatable(defaults, mt)
		return defaults
	end
end

Attribute:Class "float"
Attribute:Class "vec3"
Attribute:Class "vec4"
Attribute:Class "int"

ShaderPipeline = {}
setmetatable(ShaderPipeline, ShaderPipeline)
function ShaderPipeline.__call(...)
	return ShaderPipeline.new(...)
end

function ShaderPipeline:getUniformHandle(name)
	return shaderUniformHandleFromName(name)
end

function ShaderPipeline:getAttributeHandle(name)
	return shaderAttributeHandleFromName(name)
end

function ShaderPipeline:new(shaderPipeline)
	setmetatable(shaderPipeline, {__index = ShaderPipeline})
	shaders = {}
	for k,v in pairs(shaderPipeline) do
		if string.match(string.lower(k), "shader") then
			shaders[k] = v
		end
	end
	shaderPipeline.handle = shaderHandleFromShaders(shaders)

	for i,uniform in pairs(shaderPipeline.uniforms) do
		uniform:setHandle(shaderPipeline)
	end

	for i,attribute in pairs(shaderPipeline.attributes) do
		attribute:setHandle(shaderPipeline)
	end
	return shaderPipeline
end


sp = ShaderPipeline {
	VertexShader = [[
		#version 330 

		in vec3 vColor;
		in vec3 vPos; 
		in vec3 vNormal;

		uniform float log_depth_intermediate_factor;

		uniform mat4 model_matrix;
		uniform mat4 model_view_normal_matrix;
		uniform mat4 model_view_projection_matrix;
		uniform float hella_time;

		out vec3 fPos;
		out vec3 fColor;
		out vec3 fNormal;

		void main()
		{
			gl_Position = model_view_projection_matrix * vec4(vPos, 1);
			gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.z)) * log_depth_intermediate_factor - 1.0) * gl_Position.w;
			fPos = vec3(model_matrix * vec4(vPos, 1));
			fColor = vColor;
			fNormal = vec3(vec4(vNormal, 0.0) * model_view_normal_matrix);
		}
		]],
	FragmentShader = [[
		#version 330

		uniform vec3 uLight_pos; //Light position in world space.
		uniform vec3 uLight_col; //Light color.
		uniform vec4 uLight_attr; //Light attributes. Falloff factors, then intensity.
		uniform vec3 camera_position; //Camera position in world space.
		uniform int ambient_pass;

		uniform vec3 override_col = vec3(1.0, 1.0, 1.0);

		#define CONSTANT    0
		#define LINEAR      1
		#define EXPONENTIAL 2
		#define INTENSITY   3
		#define M_PI 3.1415926535897932384626433832795

		// set important material values
		float roughnessValue = 0.8; // 0 : smooth, 1: rough
		float F0 = 0.2; // fresnel reflectance at normal incidence
		float k = 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)

		in vec3 fPos;
		in vec3 fColor;
		in vec3 fNormal;

		out vec4 LFragment;

		float rand(vec2 co){
		    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
		}

		void point_light_fragment(in vec3 l, in vec3 v, in vec3 normal, out float specular, out float diffuse)
		{
			vec3 h = normalize(l + v); //Halfway vector.

			float base = 1 - dot(v, h);
			float exponential = pow(base, 5.0);
			float F0 = 0.5;
			float fresnel = exponential + F0 * (1.0 - exponential);

			specular = fresnel * pow(max(0.0, dot(h, normal)), 32.0);
			diffuse = max(0.0, dot(l, normal));
		}

		void point_light_fragment2(in vec3 l, in vec3 v, in vec3 normal, out float specular, out float diffuse)
		{
			vec3 h = normalize(l + v); //Halfway vector.
			
			// do the lighting calculation for each fragment.
			float NdotL = max(dot(normal, l), 0.0);
			specular = 0.0;
			if(NdotL > 0.0)
			{
				// calculate intermediary values
				float NdotH = max(dot(normal, h), 0.0); 
				float NdotV = max(dot(normal, v), 0.0); // note: this could also be NdotL, which is the same value
				float VdotH = max(dot(v, h), 0.0);
				float mSquared = roughnessValue * roughnessValue;
				
				// geometric attenuation
				float NH2 = 2.0 * NdotH;
				float g1 = (NH2 * NdotV) / VdotH;
				float g2 = (NH2 * NdotL) / VdotH;
				float geoAtt = min(1.0, min(g1, g2));
			 
				// roughness (or: microfacet distribution function)
				// beckmann distribution function
				float r1 = 1.0 / ( 4.0 * mSquared * pow(NdotH, 4.0));
				float r2 = (NdotH * NdotH - 1.0) / (mSquared * NdotH * NdotH);
				float roughness = r1 * exp(r2);
				
				// fresnel
				// Schlick approximation
				float fresnel = pow(1.0 - VdotH, 5.0);
				fresnel *= (1.0 - F0);
				fresnel += F0;
				
				specular = (fresnel * geoAtt * roughness) / (NdotV * NdotL * M_PI);
			}
			
			specular = max(0.0, NdotL * (k + specular * (1.0 - k)));
			diffuse = max(0.0, dot(l, normal));
		}

		vec3 sky_color(vec3 v, vec3 s, vec3 c)
		{
			float sun = clamp(dot(v, s), 0.0, 1.0); //sun direction, determines brightness
			return mix(vec3(0.0), vec3(0.0, 0.0, 1.0)*length(c), sun) + c*pow(sun, 2);
		}


		void main() {
			float gamma = 2.2;
			vec3 color = fColor;
			//roughnessValue = pow(0.2*length(color), 8);
			color *= override_col;
			vec3 normal = normalize(fNormal);
			//vec3 normal = normalize(cross(dFdy(fPos.xyz), dFdx(fPos.xyz)));
			vec3 final_color = vec3(0.0);
			float specular, diffuse;
			vec3 diffuse_frag = vec3(0.0);
			vec3 specular_frag = vec3(0.0);
			vec3 v = normalize(camera_position - fPos); //View vector.
			if (ambient_pass == 1) {
				vec3 l = normalize(uLight_pos-fPos);
				point_light_fragment2(l, v, normal, specular, diffuse);
				diffuse_frag = color*diffuse*sky_color(normal, normalize(vec3(0.1, 0.8, 0.1)), uLight_col);
				specular_frag = color*specular*sky_color(reflect(v, normal), normalize(vec3(0.1, 0.8, 0.1)), uLight_col);
				diffuse_frag = color*diffuse*uLight_col;
				specular_frag = color*specular*uLight_col;
			} else {
				float dist = distance(fPos, uLight_pos);
				float attenuation = uLight_attr[CONSTANT] + uLight_attr[LINEAR]*dist + uLight_attr[EXPONENTIAL]*dist*dist;
				vec3 l = normalize(uLight_pos-fPos); //Light vector.
				point_light_fragment2(l, v, normal, specular, diffuse);
				diffuse_frag = color*uLight_col*uLight_attr[INTENSITY]*diffuse/attenuation;
				specular_frag = color*uLight_col*uLight_attr[INTENSITY]*specular/attenuation;
			}

			final_color += (diffuse_frag + specular_frag);
			//vec3 fog_color = vec3(0.0, 0.0, 0.0);
			//final_color = mix(final_color, fog_color, pow(distance(fPos, camera_position)/1000, 4.0)); 

			//Tone mapping.
			//float x = 0.0001;
			//final_color = (final_color * (6.2 * x + 0.5))/(final_color * (6.2 * final_color + 1.7) + 0.06);
			final_color = final_color / (final_color + vec3(1.0));
			//final_color = final_color / (max(max(final_color.x, final_color.y), final_color.z) + 1);

			//Gamma correction.
			final_color = pow(final_color, vec3(1.0 / gamma));
			LFragment = vec4(final_color, 1.0);
		}
	]],

	uniforms = {
		log_depth_intermediate_factor = 	Uniform:float { "" },
		model_matrix = 						Uniform:mat4 { "" },
		model_view_normal_matrix = 			Uniform:mat4 { "" },
		model_view_projection_matrix = 		Uniform:mat4 { "" },
		hella_time = 						Uniform:float { "" },
		thing_scale = 						Uniform:float {  = 4.2, min = 0.0, max = 22.2 }
	},
	attributes = {
		Attribute:vec3 { "vColor" },
		Attribute:vec3 { "vPos" },
		Attribute:vec3 { "vNormal" }
	}
}
-- Uniforms {
-- 	Uniform.Float:new { arm_width = 2.85, min = 0.0, max = 1.0}
-- 	Uniform.Float:new { rotation = 543.0 }
-- 	Uniform.Float:new { diameter = 80.0 }
-- 	Uniform.Float:new { noise_scale = 4.45 }
-- 	Uniform.Float:new { noise_strength = 4.7 }
-- 	Uniform.Float:new { disk_height = 5.24 }
-- 	Uniform.Float:new { spiral_density = 2.8 }
-- 	Uniform.Float:new { bulge_mask_radius = 9.69 }
-- 	Uniform.Float:new { bulge_mask_power = 3.43 }
-- 	Uniform.Float:new { bulge_width = 23.0 }
-- 	-- samples = 50,
-- 	-- brightness = 100
-- }