-- Main Lua Scipt!
ENGINE_RunScript("assets/scripts/GameDemo/script_list.lua")
ENGINE_LoadScriptTable(ScriptList)

math.randomseed(os.time())
LoadAssets()

Ground = LoadEntity(EnvirDefs["ground"])
TallCubes = {}
for index = 0, 9 do
	TallCubes[index + 1] = LoadEntity(EnvirDefs["tall_cube_" .. index])
end
nanosuit = LoadEntity(ModelDefs["nanosuit"])

area_light_0 = LoadEntity(LightDefs["area_light_0"])
direction_light_0 = LoadEntity(LightDefs["direction_light_0"])

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