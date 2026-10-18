-- Main Lua Scipt!
ENGINE_RunScript("assets/scripts/GameDemo/script_list.lua")
ENGINE_LoadScriptTable(ScriptList)

math.randomseed(os.time())
LoadAssets()

--teapot = LoadEntity(ModelDefs["teapot"])
--sponza = LoadEntity(ModelDefs["sponza"])

--point_light_1 = LoadEntity(LightDefs["point_light_1"])
--point_light_2 = LoadEntity(LightDefs["point_light_2"])
--point_light_3 = LoadEntity(LightDefs["point_light_3"])
--point_light_4 = LoadEntity(LightDefs["point_light_4"])
--direction_light_0 = LoadEntity(LightDefs["direction_light_0"])
--area_light_0 = LoadEntity(LightDefs["area_light_0"])

light_test(1024)

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