--[[
	cube 默认边长为2
	sphere 默认半径为1
	capsule 默认半径1，半高1（总高4）
]]--

EnvirDefs = 
{
	pbr_ball = 
	{
		tag = "pbr_ball",
		group = "Envir",
		components = 
		{
			Transform = {
				position = vec3(0.0, 0.0, 0.0),
				scale = vec3(1.0, 1.0, 1.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "sphere"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(0.5, 0.0, 0.0, 1.0),
						metallic = 1.0,
						roughness = 1.0,
						ao =1.0,
						useTex = false
					}
				}
			}
		}
	},
	ball_1 = 
	{
		tag = "ball_1",
		group = "Envir",
		components = 
		{
			Transform = {
				position = vec3(5.0, 1.0, 5.0),
				scale = vec3(1.0, 1.0, 1.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "sphere"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(1.0, 1.0, 1.0, 1.0),
						metallic = 1.0,
						roughness = 1.0,
						ao =1.0,
						useTex = true,
						albedoMap = "wall_albedo",
						normalMap = "wall_normal",
						metallicMap = "wall_metallic",
						roughnessMap = "wall_roughness",
						aoMap = "wall_ao"
					}
				}
			},
			Physics = {
				type = BodyType.Dynamic,
				shape = "sphere",
				sphere_radius = 1.0
			}
		}
	},
	cube_1 = 
	{
		tag = "cube_1",
		group = "Envir",
		components = 
		{
			Transform = {
				position = vec3(-5.0, 1.0, 5.0),
				scale = vec3(1.0, 1.0, 1.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(1.0, 1.0, 1.0, 1.0),
						metallic = 1.0,
						roughness = 1.0,
						ao =1.0,
						useTex = true,
						albedoMap = "rusted_iron_albedo",
						normalMap = "rusted_iron_normal",
						metallicMap = "rusted_iron_metallic",
						roughnessMap = "rusted_iron_roughness",
						aoMap = "rusted_iron_ao"
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
				--box_halfExtents = vec3(1.0, 1.0, 1.0}
			}
		}
	},
	cube_2 = 
	{
		tag = "cube_2",
		group = "trigger",
		components = 
		{
			Transform = {
				position = vec3(-10.0, 1.0, 5.0),
				scale = vec3(0.5, 0.5, 0.5),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(0.0, 0.8, 0.0, 1.0),
						metallic = 0.5,
						roughness = 0.5,
						ao =1.0,
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
				b_Trigger = true,
				box_halfExtents = vec3(0.5, 0.5, 0.5)
			}
		}
	},
	cube_3 = 
	{
		tag = "cube_3",
		group = "moveable",
		components = 
		{
			Transform = {
				position = vec3(-5.0, 6.0, 5.0),
				scale = vec3(1.0, 1.0, 1.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(1.0, 1.0, 1.0, 1.0),
						metallic = 1.0,
						roughness = 1.0,
						ao =1.0,
						useTex = false,
						albedoMap = "",
						normalMap = "",
						metallicMap = "",
						roughnessMap = "",
						aoMap = ""
					}
				}
			}
		}
	},
	platform1 = 
	{
		tag = "platform_1",
		group = "Envir",
		components = 
		{
			Transform = {
				position = vec3(0, 2, 5),
				scale = vec3(2.0, 0.5, 2.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(1.0, 1.0, 1.0, 1.0),
						metallic = 1.0,
						roughness = 1.0,
						ao =1.0,
						useTex = true,
						albedoMap = "gold_albedo",
						normalMap = "gold_normal",
						metallicMap = "gold_metallic",
						roughnessMap = "gold_roughness",
						aoMap = "gold_ao"
					}
				}
			},
			Physics = {
				type = BodyType.Static,
				shape = "box",
				box_halfExtents = vec3(2.0, 0.5, 2.0)
			}
		}
	},
	platform2 = 
	{
		tag = "platform_2",
		group = "Envir",
		components = 
		{
			Transform = {
				position = vec3(10.0, 1, 0),
				scale = vec3(2.0, 0.25, 2.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(0.0, 0.0, 0.8, 1.0),
						metallic = 0.0,
						roughness = 1.0,
						ao =1.0,
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
				type = BodyType.Kinematic,
				shape = "box",
				box_halfExtents = vec3(2.0, 0.25, 2.0)
			}
		}
	},
	ground = 
	{
		tag = "ground",
		group = "Envir",
		components = 
		{
			Transform = {
				position = vec3(0, 0, 0),
				scale = vec3(2.0, 1.0, 2.0),
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
						ao =1.0,
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
	},
	wall1 = 
	{
		tag = "wall1",
		group = "Envir",
		components = 
		{
			Transform = {
				position = vec3(21.0, 2.5, 0),
				scale = vec3(1.0, 2.5, 20.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(1.0, 1.0, 1.0, 1.0),
						metallic = 0.0,
						roughness = 1.0,
						ao =1.0,
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
				box_halfExtents = vec3(1.0, 2.5, 20.0)
			}
		}
	},
	wall2 = 
	{
		tag = "wall2",
		group = "Envir",
		components = 
		{
			Transform = {
				position = vec3(-21.0, 2.5, 0),
				scale = vec3(1.0, 2.5, 20.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(1.0, 1.0, 1.0, 1.0),
						metallic = 0.0,
						roughness = 1.0,
						ao =1.0,
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
				box_halfExtents = vec3(1.0, 2.5, 20.0)
			}
		}
	},
	wall3 = 
	{
		tag = "wall3",
		group = "Envir",
		components = 
		{
			Transform = {
				position = vec3(0.0, 2.5, 21.0),
				scale = vec3(20.0, 2.5, 1.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(1.0, 1.0, 1.0, 1.0),
						metallic = 0.0,
						roughness = 1.0,
						ao =1.0,
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
				box_halfExtents = vec3(20.0, 2.5, 1.0)
			}
		}
	},
	wall4 = 
	{
		tag = "wall4",
		group = "Envir",
		components = 
		{
			Transform = {
				position = vec3(0.0, 2.5, -21.0),
				scale = vec3(20.0, 2.5, 1.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(1.0, 1.0, 1.0, 1.0),
						metallic = 0.0,
						roughness = 1.0,
						ao =1.0,
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
				box_halfExtents = vec3(20.0, 2.5, 1.0)
			}
		}
	}
}

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