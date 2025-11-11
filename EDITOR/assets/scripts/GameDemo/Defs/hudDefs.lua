HudDefs = 
{
	game_start =
	{
		tag = "game_start",
		group = "",
		components = 
		{
			Transform = {
				position = vec3(0.0, 0.0, 0.0),
				scale = vec3(0.75, 0.4, 1.0),
				rotation = vec3(0.0, 0.0, 0.0)
			},
			MeshFilter = {
				type = "hud_quad"
			},
			MeshRender = {
				shader = "hudShader",
				texture = "",
				color = vec4(1.0, 1.0, 1.0, 1.0)
			},
		}
	}
}