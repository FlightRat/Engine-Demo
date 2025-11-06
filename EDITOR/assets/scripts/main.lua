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
run_script("assets/scripts/GameDemo/GameState.lua")
run_script("assets/scripts/GameDemo/TitleState.lua")

math.randomseed(os.time())
LoadAssets()

gStateStack = StateStack()
local title = TitleState:Create(gStateStack)
gStateStack:change_state(title)

main = {
	[1] = {
		update = function()
			gStateStack:update(ENGINE_DeltaTime())
		end
	},
	[2] = {
		render = function()
			gStateStack:render()
		end
	},
}