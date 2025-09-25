-- Main Lua Scipt!
run_script("assets/scripts/GameDemo/utilities.lua")
run_script("assets/scripts/GameDemo/assetDefs.lua")
run_script("assets/scripts/GameDemo/entityDefs.lua")
run_script("assets/scripts/GameDemo/player.lua")
run_script("assets/scripts/GameDemo/enemy.lua")
run_script("assets/scripts/GameDemo/projectile.lua")
run_script("assets/scripts/GameDemo/collision_system.lua")

math.randomseed(os.time())

LoadAssets()

local player = LoadEntity(PlayerDefs["player"])
local floor = LoadEntity(EnvirDefs["floor"])
--local enemy1 = LoadEntity(EnemyDefs["enemy_big"])
local enemy2 = LoadEntity(EnemyDefs["enemy_small"])
te = Entity(enemy2)

gCollisionSystem = CollisionSystem:Create()


gPlayer = Player:Create({id=player, move_speed=0.2})

main = {
	[1] = {
		update = function()
			gPlayer:Update()
			gCollisionSystem:Update()
			if Keyboard.pressed(KEY_F) then
				te:kill()
			end
			--UpdateEnemy()
			--UpdateProjectile()
			--SpawnEnemies()
		end
	},
	[2] = {
		render = function()
		end
	},
}