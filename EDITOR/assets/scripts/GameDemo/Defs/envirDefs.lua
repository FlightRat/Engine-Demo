--[[
	cube 默认边长为2
	sphere 默认半径为1
	capsule 默认半径1，半高1（总高4）
]]--

EnvirDefs = 
{
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