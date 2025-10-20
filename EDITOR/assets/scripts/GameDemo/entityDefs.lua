ObjDefs =
{
	ball = 
	{
		tag = "ball",
		group = "",
		components = 
		{
			Transform = {
				position = {x=0.0, y=5.0, z=5.0},
				scale = {x=0.5, y=0.5, z=0.5},
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
				sphere_radius = 0.5
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
				position = {x=0.0, y=5.0, z=-5.0},
				scale = {x=0.5, y=0.5, z=0.5},
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
				box_halfExtents = vec3(0.5, 0.5, 0.5)
			}
		}
	},
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
				scale = {x=1.0, y=1.0, z=1.0},
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
				box_halfExtents = vec3(10.0, 0.0005, 10.0)
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