-- Main Lua Scipt!

run_script("assets/scripts/GameDemo/entityDefs.lua")
run_script("assets/scripts/GameDemo/assetDefs.lua")
run_script("assets/scripts/GameDemo/utilities.lua")

LoadAssets()

local player = LoadEntity(PlayerDefs["player"])
local floor = LoadEntity(EnvirDefs["floor"])
local enemy1 = LoadEntity(EnemyDefs["enemy_big"])
local enemy2 = LoadEntity(EnemyDefs["enemy_small"])

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