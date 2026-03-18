LightDefs = 
{
	point_light_0 = 
	{
		tag = "point_light_0",
		group = "light",
		components =
		{
			Light = {
				diffuse = vec3(0.8, 0.8, 0.8),
				specular = vec3(1.0, 1.0, 1.0),
				ambient = vec3(0.05, 0.05, 0.05),

				type = "point_light",

				pos = vec3(0.0, 2.5, 0.0),
				constant = 1.0,
				linear = 0.5,
				quadratic = 0.05,
				render = true
			}
		}
	},
	point_light_1 = 
	{
		tag = "point_light_1",
		group = "light",
		components =
		{
			Light = {
				diffuse = vec3(0.8, 0.8, 0.8),
				specular = vec3(1.0, 1.0, 1.0),
				ambient = vec3(0.05, 0.05, 0.05),

				type = "point_light",

				pos = vec3(10.0, 2.5, 10.0),
				constant = 1.0,
				linear = 0.5,
				quadratic = 0.05,
				render = true
			}
		}
	},
	point_light_2 = 
	{
		tag = "point_light_2",
		group = "light",
		components =
		{
			Light = {
				diffuse = vec3(0.8, 0.8, 0.8),
				specular = vec3(1.0, 1.0, 1.0),
				ambient = vec3(0.05, 0.05, 0.05),

				type = "point_light",

				pos = vec3(-10.0, 2.5, 10.0),
				constant = 1.0,
				linear = 0.5,
				quadratic = 0.05,
				render = true
			}
		}
	},
	point_light_3 = 
	{
		tag = "point_light_3",
		group = "light",
		components =
		{
			Light = {
				diffuse = vec3(0.8, 0.8, 0.8),
				specular = vec3(1.0, 1.0, 1.0),
				ambient = vec3(0.05, 0.05, 0.05),

				type = "point_light",

				pos = vec3(10.0, 2.5, -10.0),
				constant = 1.0,
				linear = 0.5,
				quadratic = 0.05,
				render = true
			}
		}
	},
	point_light_4 = 
	{
		tag = "point_light_4",
		group = "light",
		components =
		{
			Light = {
				diffuse = vec3(0.8, 0.8, 0.8),
				specular = vec3(1.0, 1.0, 1.0),
				ambient = vec3(0.05, 0.05, 0.05),

				type = "point_light",

				pos = vec3(-10.0, 2.5, -10.0),
				constant = 1.0,
				linear = 0.5,
				quadratic = 0.05,
				render = true
			}
		}
	},
	direction_light_1 = 
	{
		tag = "direction_light_1",
		group = "light",
		components =
		{
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