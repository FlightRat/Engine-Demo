-- Missing default-demo definitions restored from commit 3cc9941.
-- Existing definitions take precedence; other Scenes keep their current data.

EnvirDefs.pbr_ball = EnvirDefs.pbr_ball or {
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
}

EnvirDefs.rust_ball = EnvirDefs.rust_ball or {
	tag = "rust_ball",
	group = "Envir",
	components =
	{
		Transform = {
			position = vec3(-5.0, 2.0, -10.0),
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
					albedoMap = "rusted_iron_albedo",
					normalMap = "rusted_iron_normal",
					metallicMap = "rusted_iron_metallic",
					roughnessMap = "rusted_iron_roughness",
					aoMap = "rusted_iron_ao"
				}
			}
		}
	}
}

EnvirDefs.gold_ball = EnvirDefs.gold_ball or {
	tag = "gold_ball",
	group = "Envir",
	components =
	{
		Transform = {
			position = vec3(-3.0, 2.0, -10.0),
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
					albedoMap = "gold_albedo",
					normalMap = "gold_normal",
					metallicMap = "gold_metallic",
					roughnessMap = "gold_roughness",
					aoMap = "gold_ao"
				}
			}
		}
	}
}

EnvirDefs.grass_ball = EnvirDefs.grass_ball or {
	tag = "grass_ball",
	group = "Envir",
	components =
	{
		Transform = {
			position = vec3(-1.0, 2.0, -10.0),
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
					albedoMap = "grass_albedo",
					normalMap = "grass_normal",
					metallicMap = "grass_metallic",
					roughnessMap = "grass_roughness",
					aoMap = "grass_ao"
				}
			}
		}
	}
}

EnvirDefs.plastic_ball = EnvirDefs.plastic_ball or {
	tag = "plastic_ball",
	group = "Envir",
	components =
	{
		Transform = {
			position = vec3(1.0, 2.0, -10.0),
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
					albedoMap = "plastic_albedo",
					normalMap = "plastic_normal",
					metallicMap = "plastic_metallic",
					roughnessMap = "plastic_roughness",
					aoMap = "plastic_ao"
				}
			}
		}
	}
}

EnvirDefs.wall_ball = EnvirDefs.wall_ball or {
	tag = "wall_ball",
	group = "Envir",
	components =
	{
		Transform = {
			position = vec3(3.0, 2.0, -10.0),
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
		}
	}
}

EnvirDefs.ball_1 = EnvirDefs.ball_1 or {
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
}

EnvirDefs.cube_1 = EnvirDefs.cube_1 or {
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
}

EnvirDefs.cube_2 = EnvirDefs.cube_2 or {
	tag = "cube_2",
	group = "Envir",
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
}

EnvirDefs.platform1 = EnvirDefs.platform1 or {
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
}

EnvirDefs.platform2 = EnvirDefs.platform2 or {
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
}

EnvirDefs.wall1 = EnvirDefs.wall1 or {
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
}

EnvirDefs.wall2 = EnvirDefs.wall2 or {
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
}

EnvirDefs.wall3 = EnvirDefs.wall3 or {
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
}

EnvirDefs.wall4 = EnvirDefs.wall4 or {
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

ModelDefs.cyborg = ModelDefs.cyborg or {
	tag = "cyborg",
	group = "Models",
	components =
	{
		Transform = {
			position = vec3(-10, 0.0, -15.0),
			scale = vec3(2.0, 2.0, 2.0),
			rotation = vec3(0.0, 0.0, 0.0)
		},
		MeshFilter = {
			type = "cyborg"
		},
		MeshRender = {
			material = {
			}
		}
	}
}

ModelDefs.nanosuit = ModelDefs.nanosuit or {
	tag = "nanosuit",
	group = "Models",
	components =
	{
		Transform = {
			position = vec3(10.0, 0.0, -15.0),
			scale = vec3(0.5, 0.5, 0.5),
			rotation = vec3(0.0, 0.0, 0.0)
		},
		MeshFilter = {
			type = "nanosuit"
		},
		MeshRender = {
			material = {
			}
		}
	}
}

LightDefs.direction_light_1 = LightDefs.direction_light_1 or {
	tag = "direction_light_1",
	group = "light",
	components =
	{
		Light = {
			color = vec3(1.0, 1.0, 1.0),
			intensity = 10.0,

			type = "direction_light",

			direction = vec3(0, -1.0, 0)
		}
	}
}

