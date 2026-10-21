--[[
	cube 默认边长为2
	sphere 默认半径为1
	capsule 默认半径1，半高1（总高4）
]]--

EnvirDefs = {}

EnvirDefs.ground = {
	tag = "ground",
	group = "Envir",
	components = {
		Transform = {
			position = vec3(0.0, 0.0, 0.0),
			scale = vec3(20.0, 1.0, 20.0),
			rotation = vec3(0.0, 0.0, 0.0)
		},
		MeshFilter = {
			type = "plane"
		},
		MeshRender = {
			material = {
				{
					shadingModel = "PBR",
					color = vec4(1.0, 1.0, 1.0, 1.0),
					metallic = 1.0,
					roughness = 1.0,
					ao = 1.0,
					useTex = false,
					albedoMap = "",
					normalMap = "",
					metallicMap = "",
					roughnessMap = "",
					aoMap = ""
				}
			}
		},
		Physics = {
			type = BodyType.Static,
			shape = "box",
			box_halfExtents = vec3(20.0, 0.0005, 20.0)
		}
	}
}

local function makeTallCube(index)
	return {
		tag = "tall_cube_" .. index,
		group = "Envir",
		components = {
			Transform = {
				position = vec3((index - 4.5) * 3.0, 2.5, 0.0),
				scale = vec3(1.0, 5.0, 1.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(0.5, 0.5, 0.5, 1.0),
						metallic = 0.0,
						roughness = 1.0,
						ao = 1.0,
						useTex = false,
						albedoMap = "",
						normalMap = "",
						metallicMap = "",
						roughnessMap = "",
						aoMap = ""
					}
				}
			},
			Physics = {
				type = BodyType.Dynamic,
				shape = "box",
				box_halfExtents = vec3(1.0, 1.0, 1.0),
				mass = 5.0,
				linear_damping = 2.0,
				angular_damping = 2.0
			}
		}
	}
end

for index = 0, 9 do
	EnvirDefs["tall_cube_" .. index] = makeTallCube(index)
end

function DisplayPBR()
	local nrRows = 7
	local nrColumns = 7
	local spacing = 2.5
	for row = 0, nrRows - 1 do
		local metallic = row / (nrRows - 1)
		for col = 0, nrColumns - 1 do
			local ball_id = LoadEntity(EnvirDefs["pbr_ball"])
			local ball_entity = Entity(ball_id)
			local transform = ball_entity:get_component(Transform)
			local meshRender = ball_entity:get_component(MeshRender)
			local material = meshRender:get_material(0)
			-- 设置位置：网格居中排列
			local x = (col - (nrColumns / 2)) * spacing
			local y = (row - (nrRows / 2)) * spacing + 20.0
			local z = -40.0
			transform.position = vec3(x, y, z)

			-- 设置粗糙度：按列渐变 0.05 → 1.0
			local roughness = col / (nrColumns - 1)
			roughness = math.max(roughness, 0.05)  -- 替代 clamp，Lua 标准写法

			-- 赋值 PBR 参数
			material.metallic = metallic
			material.roughness = roughness
		end
	end
end

function DisplayPBR_tex()
	LoadEntity(EnvirDefs["gold_ball"])
	LoadEntity(EnvirDefs["wall_ball"])
	LoadEntity(EnvirDefs["plastic_ball"])
	LoadEntity(EnvirDefs["grass_ball"])
	LoadEntity(EnvirDefs["rust_ball"])
end

function light_test(light_count)
	LoadEntity({
		tag = "light_test_ground",
		group = "light_test",
		components = {
			Transform = {
				position = vec3(0.0, -0.2, 0.0),
				scale = vec3(64.0, 0.2, 64.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(0.6, 0.6, 0.6, 1.0),
						metallic = 0.0,
						roughness = 0.8,
						ao = 1.0,
						useTex = false
					}
				}
			}
		}
	})

	for index = 1, light_count do
		local position = vec3(math.random() * 60.0 - 30.0, 1.5, math.random() * 60.0 - 30.0)
		LoadEntity({
			tag = "light_test_" .. index,
			group = "light_test",
			components = {
				Transform = {
					position = position,
					scale = vec3(1.0, 1.0, 1.0),
					rotation = vec3(0.0, 0.0, 0.0)
				},
				Light = {
					color = vec3(0.2 + math.random() * 0.8, 0.2 + math.random() * 0.8, 0.2 + math.random() * 0.8),
					intensity = 3.0,
					type = "point_light",
					pos = position,
					constant = 1.0,
					linear = 0.09,
					quadratic = 0.032,
					range = 5.0,
					render = false
				}
			}
		})
	end
end