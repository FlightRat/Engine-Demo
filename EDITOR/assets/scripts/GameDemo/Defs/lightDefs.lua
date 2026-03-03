LightDefs = 
{
	point_light_1 = 
	{
		tag = "point_light_1",
		group = "light",
		components =
		{
			Transform = {
				position = vec3(0.0, 20.0, 0.0),
				scale = vec3(0.5, 0.5, 0.5),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "sphere"
			},
			MeshRender = {
				shader = "mainShader",
				color = vec4(1.0, 1.0, 1.0, 1.0),
				useTex = false
			},
			Light = {
				diffuse = vec3(0.8, 0.8, 0.8),
				specular = vec3(1.0, 1.0, 1.0),
				ambient = vec3(0.05, 0.05, 0.05),

				type = "point_light",

				pos = vec3(0.0, 20.0, 0.0),
				constant = 1.0,
				linear = 0.09,
				quadratic = 0.032
			}
		}
	},
	direction_light_1 = 
	{
		tag = "direction_light_1",
		group = "light",
		components =
		{
			Transform = {
				position = vec3(0.0, 20.0, 0.0),
				scale = vec3(0.5, 0.5, 0.5),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			Light = {
				diffuse = vec3(0.4, 0.4, 0.4),
				specular = vec3(0.5, 0.5, 0.5),
				ambient = vec3(0.05, 0.05, 0.05),

				type = "direction_light",

				direction = vec3(-0.2, -1.0, -0.3)
			}
		}
	}
}