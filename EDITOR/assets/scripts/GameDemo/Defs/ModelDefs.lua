ModelDefs = 
{
	nanosuit = 
	{
		tag = "nanosuit",
		group = "Models",
		components = 
		{
			Transform = {
				position = vec3(0.0, 0.0, 0.0),
				scale = vec3(0.5, 0.5, 0.5),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "nanosuit"
			},
			MeshRender = {
				material = {
				}
			}
		}
	},
	backpack = 
	{
		tag = "backpack",
		group = "Models",
		components = 
		{
			Transform = {
				position = vec3(2.0, 2.0, -15.0),
				scale = vec3(1.0, 1.0, 1.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "backpack"
			},
			MeshRender = {
				flip_uv = true,
				material = {
				}
			}
		}
	},
	cyborg = 
	{
		tag = "cyborg",
		group = "Models",
		components = 
		{
			Transform = {
				position = vec3(-10, 0.0, -15.0),
				scale = vec3(2.0, 2.0, 2.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "cyborg"
			},
			MeshRender = {
				material = {
				}
			}
		}
	},
	owl = 
	{
		tag = "owl",
		group = "Models",
		components = 
		{
			Transform = {
				position = vec3(0.0, 5.0, -15.0),
				scale = vec3(5.0, 5.0, 5.0),
				rotation = vec3(90.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "owl"
			},
			MeshRender = {
				material = {
				}
			}
		}
	}
}