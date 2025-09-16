-- Main Lua Scipt!

run_script("assets/scripts/GameDemo/entityDefs.lua")
run_script("assets/scripts/GameDemo/assetDefs.lua")
run_script("assets/scripts/GameDemo/utilities.lua")

LoadAssets()

local player = LoadEntity(PlayerDefs["player"])
local floor = LoadEntity(EnvirDefs["floor"])
local enemy1 = LoadEntity(EnemyDefs["enemy_big"])
local enemy2 = LoadEntity(EnemyDefs["enemy_small"])

local player_transform = player:get_component(Transform)

local x_pos = 0.0
local z_pos = 0.0

main = {
	[1] = {
		update = function()
			-- Keyboard
			if Keyboard.pressed(KEY_W) then
				z_pos = z_pos - 0.2
			end
			if Keyboard.pressed(KEY_A) then
				x_pos = x_pos - 0.2
			end
			if Keyboard.pressed(KEY_S) then
				z_pos = z_pos + 0.2
			end
			if Keyboard.pressed(KEY_D) then
				x_pos = x_pos + 0.2
			end
			player_transform.position.x = x_pos
			player_transform.position.z = z_pos
		end
	},
	[2] = {
		render = function()
			
		end
	},
}