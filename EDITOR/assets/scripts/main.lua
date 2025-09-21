-- Main Lua Scipt!

run_script("assets/scripts/GameDemo/entityDefs.lua")
run_script("assets/scripts/GameDemo/assetDefs.lua")
run_script("assets/scripts/GameDemo/utilities.lua")
run_script("assets/scripts/GameDemo/player.lua")
run_script("assets/scripts/GameDemo/enemy.lua")
run_script("assets/scripts/GameDemo/collision_system.lua")

math.randomseed(os.time())

LoadAssets()

local player = LoadEntity(PlayerDefs["player"])
local floor = LoadEntity(EnvirDefs["floor"])
--local enemy1 = LoadEntity(EnemyDefs["enemy_big"])
--local enemy2 = LoadEntity(EnemyDefs["enemy_small"])

gCollisionSystem = CollisionSystem:Create()

gPlayer = Player:Create({id=player})

main = {
	[1] = {
		update = function()
			gPlayer:Update()
			gCollisionSystem:Update()
			UpdateEnemy()
			SpawnEnemies()
		end
	},
	[2] = {
		render = function()
		end
	},
}