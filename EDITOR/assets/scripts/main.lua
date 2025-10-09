-- Main Lua Scipt!
run_script("assets/scripts/GameDemo/utilities.lua")
run_script("assets/scripts/GameDemo/assetDefs.lua")
run_script("assets/scripts/GameDemo/entityDefs.lua")
run_script("assets/scripts/GameDemo/player.lua")
run_script("assets/scripts/GameDemo/enemy.lua")
run_script("assets/scripts/GameDemo/projectile.lua")
run_script("assets/scripts/GameDemo/collision_system.lua")
run_script("assets/scripts/GameDemo/game_data.lua")
run_script("assets/scripts/GameDemo/hud.lua")

math.randomseed(os.time())

LoadAssets()

--local player = LoadEntity(PlayerDefs["player"])
--local floor = LoadEntity(EnvirDefs["floor"])
--local enemy_s = Enemy:Create("enemy_small")
--local enemy_b = Enemy:Create("enemy_big")
--AddEnemy(enemy_s)
--AddEnemy(enemy_b)

--gCollisionSystem = CollisionSystem:Create()
--gHud = Hud:Create()

--gPlayer = Player:Create({id=player, move_speed=0.2})

Music.play("2:23am")

print("Score: "..0)

main = {
	[1] = {
		update = function()
		end
	},
	[2] = {
		render = function()
		end
	},
}