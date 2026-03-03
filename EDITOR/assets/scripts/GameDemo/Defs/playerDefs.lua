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
				position = vec3(0.0, 1.0, 0.0),
				scale = vec3(0.5, 0.5, 0.5),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "capsule"
			},
			MeshRender = {
				shader = "mainShader",
				color = vec4(0.678, 0.847, 1.0, 1.0),
				useTex = false
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
				position = vec3(0.0, 1.0, 0.5),
				scale = vec3(1.0, 0.2, 0.5),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "cube"
			},
			MeshRender = {
				shader = "mainShader",
				color = vec4(1.0, 0.647, 0.0, 1.0),
				useTex = false
			}
		}
	}
}