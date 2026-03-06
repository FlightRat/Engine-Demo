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
				material = {
					{
						shaderName = "mainShader",
						color = vec4(1.0, 0.0, 0.0, 1.0),
						shininess = 64.0,
						useTex = false,
						diffuse = "",
						specular = ""
					}
				}
			},
			Physics = {
				type = BodyType.Dynamic, --???
				shape = "sphere",
				sphere_radius = 0.1,
				b_Trigger = true,
				enable_gravity = false,
				linear_axis_factor = vec3 (1.0, 0.0, 1.0),
				angular_axis_factor = vec3(0.0, 0.0, 0.0)
			}
		}
	}
}