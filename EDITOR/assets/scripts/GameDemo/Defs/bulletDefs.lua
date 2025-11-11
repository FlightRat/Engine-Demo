BulletDefs = 
{
	normal_shot =
	{
		tag = "normal_shot",
		group = "bullet",
		components =
		{
			Transform = {
				position = vec3(0.0, 0.0, 0.0),
				scale = vec3(0.1, 0.1, 0.1),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "sphere"
			},
			MeshRender = {
				shader = "colorShader",
				texture = "",
				color = vec4(1.0, 0.0, 0.0, 1.0)
			},
			Physics = {
				type = BodyType.Dynamic, --???
				shape = "sphere",
				sphere_radius = 0.1,
				b_Trigger = true,
				enable_gravity = false,
				linear_axis_factor = vec3 (1.0, 0.0, 0.0),
				angular_axis_factor = vec3(0.0, 0.0, 0.0)
			}
		}
	}
}