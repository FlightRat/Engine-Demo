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
				shader = "colorShader",
				texture = "",
				color = vec4(1.0, 1.0, 0.0, 1.0)
			},
			Light = {
				diffuse = vec3(1.0, 1.0, 1.0),
				specular = vec3(1.0, 1.0, 1.0),
				ambient = vec3(1.0, 1.0, 1.0),

				type = "point_light",

				pos = vec3(0.0, 20.0, 0.0),
				constant = 1.0,
				linear = 1.0,
				quadratic = 1.0
			}
		}
	}
}