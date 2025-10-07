EnemyDefs =
{
	enemy_big = 
	{
		tag = "big",
		group = "enemy",
		components = 
		{
			Transform = {
				position = {x=-5.0, y=0.0, z=0.0},
				scale = {x=0.75, y=0.5, z=0.75},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			Mesh = {
				type = "cube",
				shader = "colorShader",
				color = {R=1.0, G=0.0, B=0.0, A=1.0}
			},
			CubeCollider = {
				width = 0.75,
				height = 0.75
			}
		},
		max_speed = 0.0,--0.3
		random_pos = false,
	},
	enemy_small = 
	{
		tag = "small",
		group = "enemy",
		components = 
		{
			Transform = {
				position = {x=0.0, y=0.0, z=-5.0},
				scale = {x=0.25, y=0.5, z=0.25},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			Mesh = {
				type = "cube",
				shader = "colorShader",
				color = {R=0.0, G=0.0, B=1.0, A=1.0}
			},
			CubeCollider = {
				width = 0.25,
				height = 0.25
			}
		},
		max_speed = 0.0,--0.1
		random_pos = false
	},
}

PlayerDefs = 
{
	player = 
	{
		tag = "player",
		components = 
		{
			Transform = {
				position = {x=0.0, y=0.0, z=0.0},
				scale = {x=0.5, y=0.5, z=0.5},
				rotation = {x=0.0, y=-90.0, z=0.0} -- (-90бу) for move with rotation
			},
			Mesh = {
				type = "cube",
				shader = "colorShader",
				color = {R=0.0, G=1.0, B=0.0, A=1.0},
				texture = 2
			},
			CubeCollider = {
				width = 0.5,
				height = 0.5
			}
		}
	}
}

EnvirDefs = 
{
	floor = 
	{
		tag = "floor",
		group = "Envir",
		components = 
		{
			Transform = {
				position = {x=0, y=0, z=0},
				scale = {x=1.0, y=1.0, z=1.0},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			Mesh = {
				type = "plane",
				shader = "texShader",
				color = {R=1.0, G=1.0, B=1.0, A=1.0},
				texture = 1
			}
		}
	}
}

ProjectileDefs = 
{
	proj_1 = 
	{
		group = "projectiles",
		components = 
		{
			Transform = {
				position = {x=0, y=0, z=0},
				scale = {x=0.1, y=0.5, z=0.1},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			Mesh = {
				type = "cube",
				shader = "colorShader",
				color = {R=1.0, G=0.0, B=1.0, A=1.0}
			},
			CubeCollider = {
				width = 0.1,
				height = 0.1
			}
		},
		life_time = 2000,
		proj_speed = 0.1
	}
}

HudDefs = 
{
	lives =
	{
		group = "lives",
		components = 
		{
			Transform = {
				position = {x=0, y=0, z=0},
				scale = {x=0.05, y=0.05, z=0.05},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			Mesh = {
				type = "hud_quad",
				shader = "hudShader",
				color = {R=1.0, G=1.0, B=1.0, A=1.0}
			}
		}
	},
	game_over =
	{
		tag = "game_over",
		components = 
		{
			Transform = {
				position = {x=0, y=0, z=0},
				scale = {x=0.75, y=0.4, z=1.0},
				rotation = {x=0.0, y=0.0, z=0.0}
			},
			Mesh = {
				type = "hud_quad",
				shader = "hudShader",
				color = {R=1.0, G=1.0, B=1.0, A=1.0},
				bHidden = true
			}
		}
	}
}