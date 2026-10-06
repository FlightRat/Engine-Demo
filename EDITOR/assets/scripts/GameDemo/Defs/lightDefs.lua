LightDefs = 
{
	area_light_0 =
	{
		tag = "area_light_0",
		group = "light",
		components =
		{
			Light = {
				color = vec3(1.0, 1.0, 1.0),
				intensity = 10.0,

				type = "area_light",

				half_width = 0.25,
				half_height = 0.25,
				pos = vec3(0.0, 10.0, 2.5),
				rotation = vec3(45.0, 0.0, 0.0)
			}
		}
	},
	point_light_0 = 
	{
		tag = "point_light_0",
		group = "light",
		components =
		{
			Light = {
				color = vec3(1.0, 1.0, 1.0),
				intensity = 10.0,

				type = "point_light",

				pos = vec3(0.0, 10.0, -5.0),
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
				color = vec3(1.0, 1.0, 1.0),
				intensity = 10.0,

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
				color = vec3(1.0, 1.0, 1.0),
				intensity = 10.0,

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
				color = vec3(1.0, 1.0, 1.0),
				intensity = 10.0,

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
				color = vec3(1.0, 1.0, 1.0),
				intensity = 10.0,

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
				color = vec3(1.0, 1.0, 1.0),
				intensity = 10.0,

				type = "direction_light",

				direction = vec3(0, -1.0, 0)
			}
		}
	}
}