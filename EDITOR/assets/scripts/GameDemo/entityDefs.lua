EnemyDefs =
{
	enemy_big = 
	{
		group = "enemy",
		components = 
		{
			Transform = {
				position = {x=-5, y=0.5, z=0},
				scale = {x=1, y=1, z=1}
			},
			Mesh = {
				color = {R=1.0, G=0.0, B=0.0},
				type = "cube"
			}
		}
	},
	enemy_small = 
	{
		group = "enemy",
		components = 
		{
			Transform = {
				position = {x=-5, y=5, z=0},
				scale = {x=0.5, y=0.5, z=0.5}
			},
			Mesh = {
				color = {R=0.0, G=0.0, B=1.0},
				type = "cube"
			}
		}
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
				position = {x=0, y=0.375, z=0},
				scale = {x=0.75, y=0.75, z=0.75}
			},
			Mesh = {
				color = {R=0.0, G=1.0, B=0.0},
				type = "cube"
			}
		}
	}
}

EnvirDefs = 
{
	floor = 
	{
		group = "Envir",
		components = 
		{
			Transform = {
				position = {x=0, y=0, z=0},
				scale = {x=1.0, y=1.0, z=1.0}
			},
			Mesh = {
				color = {R=1.0, G=1.0, B=1.0},
				type = "plane"
			}
		}
	}
}