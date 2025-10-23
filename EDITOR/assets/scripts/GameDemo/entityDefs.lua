--[[
	cube 默认边长为2
	sphere 默认半径为1
	capsule 默认半径1，半高1（总高4）
]]--

PlayerDefs =
{
	body=
	{
		tag = "body",
		group = "player",
		components = 
		{
			Transform = {
				position = {x=0.0, y=1.0, z=0.0},
				scale = {x=0.5, y=0.5, z=0.5},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			MeshFilter = {
				type = "capsule"
			},
			MeshRender = {
				shader = "colorShader",
				color = {R=0.678, G=0.847, B=902.0, A=1.0},
				texture = 0
			},
			Physics = {
				type = BodyType.Dynamic,
				shape = "capsule",
				capsule_halfHeight = 0.5,
				capsule_radius = 0.5,
				friction = 0.3,
				linear_damping = 5.0,
				angular_damping = 10.0,
				angular_axis_factor = vec3(0.0, 1.0, 0.0)--lock rotation only in y
			}
		}
	},
	glass = 
	{
		tag = "glass",
		group = "player",
		components = 
		{
			Transform = {
				position = {x=0.0, y=1.5, z=0.25},
				scale = {x=0.5, y=0.1, z=0.25},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				shader = "colorShader",
				color = {R=1.0, G=0.647, B=0.0, A=1.0},
				texture = 0
			}
		}
	}
}

ObjDefs =
{
	ball = 
	{
		tag = "glass",
		group = "player",
		components = 
		{
			Transform = {
				position = {x=5.0, y=1.0, z=0.0},
				scale = {x=1.0, y=1.0, z=1.0},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			MeshFilter = {
				type = "sphere"
			},
			MeshRender = {
				shader = "texShader",
				color = {R=1.0, G=1.0, B=1.0, A=1.0},
				texture = 2
			},
			Physics = {
				type = BodyType.Dynamic,
				shape = "sphere",
				sphere_radius = 1.0
			}
		}
	},
	cube = 
	{
		tag = "cube",
		group = "",
		components = 
		{
			Transform = {
				position = {x=-5.0, y=1.0, z=0.0},
				scale = {x=1.0, y=1.0, z=1.0},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				shader = "texShader",
				color = {R=1.0, G=1.0, B=1.0, A=1.0},
				texture = 1
			},
			Physics = {
				type = BodyType.Dynamic,
				shape = "box",
				box_halfExtents = vec3(1.0, 1.0, 1.0)
			}
		}
	}
}

EnvirDefs = 
{
	floor = 
	{
		tag = "floor",
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
			}
		}
	},
	platform1 = 
	{
		tag = "platform1",
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
			}
		}
	}
}

ProjectileDefs = 
{

}

HudDefs = 
{

}