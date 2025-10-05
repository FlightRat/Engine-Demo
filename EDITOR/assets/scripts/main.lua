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

local player = LoadEntity(PlayerDefs["player"])
local floor = LoadEntity(EnvirDefs["floor"])
--local enemy1 = LoadEntity(EnemyDefs["enemy_big"])
--local enemy2 = LoadEntity(EnemyDefs["enemy_small"])

gCollisionSystem = CollisionSystem:Create()
gHud = Hud:Create()

gPlayer = Player:Create({id=player, move_speed=0.2})

Music.play("2:23am")

print("Score: "..0)

main = {
	[1] = {
		update = function()
			gPlayer:Update()
			gCollisionSystem:Update()
			gHud:Update()
			UpdateEnemy()
			UpdateProjectile()
			if not gData:IsGameOver() then
				SpawnEnemy()
			else
				if Keyboard.pressed(KEY_R) then
					gData:Reset()
					gHud:Reset()
					gPlayer:Reset()
					ResetEnemy()
					ResetProjectile()
				end
			end
		end
	},
	[2] = {
		render = function()
		end
	},
}