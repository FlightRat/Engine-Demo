-- Main Lua Scipt!
ENGINE_RunScript("assets/scripts/GameDemo/script_list.lua")
ENGINE_LoadScriptTable(ScriptList)

math.randomseed(os.time())
LoadAssets()

-- state stack is used for storaging and changing scene/state
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