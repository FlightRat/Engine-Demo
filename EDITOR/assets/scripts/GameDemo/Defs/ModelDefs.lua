local sponzaMaterials = {}

for i = 1, 746 do
    sponzaMaterials[i] = {
        shadingModel = "PBR",
        color = vec4(0.7, 0.7, 0.7, 1.0),
        metallic = 0.0,
        roughness = 0.8,
        ao = 1.0,
        useTex = false
    }
end

ModelDefs = 
{
	teapot = 
	{
		tag = "teapot",
		group = "Models",
		components = 
		{
			Transform = {
				position = vec3(0.0, 0.0, 0.0),
				scale = vec3(0.5, 0.5, 0.5),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "teapot"
			},
			MeshRender = {
				material = {
					{
						shadingModel = "PBR",
						color = vec4(1.0, 0.5, 0.0, 1.0),
						metallic = 0.0,
						roughness = 1.0,
						ao = 1.0,
						useTex = false,
						albedoMap = "",
						normalMap = "",
						metallicMap = "",
						roughnessMap = "",
						aoMap = ""
					}
				}
			}
		}
	},
	sponza = 
	{
		tag = "sponza",
		group = "Models",
		components = 
		{
			Transform = {
				position = vec3(0.0, 0.0, 0.0),
				scale = vec3(1.0, 1.0, 1.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "sponza"
			},
			MeshRender = {
				material = sponzaMaterials
			}
		}
	},
}