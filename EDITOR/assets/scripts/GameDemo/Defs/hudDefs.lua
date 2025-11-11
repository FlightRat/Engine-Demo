HudDefs = 
{
	game_start =
	{
		tag = "game_start",
		group = "",
		components = 
		{
			Transform = {
				position = {x=0, y=0, z=0},
				scale = {x=0.75, y=0.4, z=1.0},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			MeshFilter = {
				type = "hud_quad"
			},
			MeshRender = {
				shader = "hudShader",
				texture = "",
				color = {R=1.0, G=1.0, B=1.0, A=1.0}
			},
		}
	}
}