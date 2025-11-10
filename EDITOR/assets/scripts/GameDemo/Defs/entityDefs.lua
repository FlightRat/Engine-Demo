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
		group = "",
		components = 
		{
			Transform = {
				position = {x=5.0, y=1.0, z=5.0},
				scale = {x=1.0, y=1.0, z=1.0},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			MeshFilter = {
				type = "sphere"
			},
			MeshRender = {
				shader = "colorShader",
				color = {R=0.0, G=1.0, B=0.0, A=1.0},
				texture = 2
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
		group = "",
		components = 
		{
			Transform = {
				position = {x=-5.0, y=1.0, z=5.0},
				scale = {x=1.0, y=1.0, z=1.0},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				shader = "colorShader",
				color = {R=0.0, G=0.0, B=1.0, A=1.0},
				texture = 1
			},
			Physics = {
				type = BodyType.Dynamic,
				shape = "box",
				box_halfExtents = vec3(1.0, 1.0, 1.0),
				mass = 5.0,
				linear_damping = 2.0,
				angular_damping = 2.0
				--box_halfExtents = {x=1.0, y=1.0, z=1.0}
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
				position = {x=-10.0, y=1.0, z=5.0},
				scale = {x=0.5, y=0.5, z=0.5},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				shader = "colorShader",
				color = {R=0.0, G=1.0, B=0.0, A=1.0},
				texture = 0
			},
			Physics = {
				type = BodyType.Static,
				shape = "box",
				b_Trigger = true,
				box_halfExtents = vec3(0.5, 0.5, 0.5)
				--box_halfExtents = {x=1.0, y=1.0, z=1.0}
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
				position = {x=0, y=2, z=5},
				scale = {x=2.0, y=0.5, z=2.0},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				shader = "colorShader",
				color = {R=1.0, G=0.0, B=0.0, A=1.0},
				texture = 0
			},
			Physics = {
				type = BodyType.Static,
				shape = "box",
				box_halfExtents = vec3(2.0, 0.5, 2.0)
				--box_halfExtents = {x=2.0, y=0.5, z=2.0}
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
				position = {x=0, y=0, z=0},
				scale = {x=2.0, y=1.0, z=2.0},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			MeshFilter = {
				type = "plane"
			},
			MeshRender = {
				shader = "colorShader",
				color = {R=1.0, G=1.0, B=1.0, A=1.0},
				texture = 1
			},
			Physics = {
				type = BodyType.Static,
				shape = "box",
				box_halfExtents = vec3(20.0, 0.0005, 20.0)
				--box_halfExtents = {x=20.0, y=0.0005, z=20.0}
			}
		}
	}
}

ProjectileDefs = 
{

}

HudDefs = 
{
	game_start =
	{
		tag = "game_start",
		group = "",
		components = 
		{
			Transform = {
				position = {x=0, y=0, z=0},
				scale = {x=0.75, y=0.4, z=1.0},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			MeshFilter = {
				type = "hud_quad"
			},
			MeshRender = {
				shader = "hudShader",
				color = {R=1.0, G=1.0, B=1.0, A=1.0},
				texture = 0
			},
		}
	}
}