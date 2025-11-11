assert(MoveState, "MoveState does not exist!")

CharacterStates = 
{
	move = MoveState
}

PlayerDefs =
{
	body=
	{
		tag = "body",
		group = "player",
		controller = {"move"}, -- define the states of the entity, like "move" & "jump"
		default_state = "move",
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
				texture = "",
				color = {R=0.678, G=0.847, B=1.0, A=1.0}
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
				position = {x=0.0, y=1.0, z=0.5},
				scale = {x=1.0, y=0.2, z=0.5},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				shader = "colorShader",
				texture = "",
				color = {R=1.0, G=0.647, B=0.0, A=1.0}
			}
		}
	}
}