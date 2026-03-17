-- Main Lua Scipt!
ENGINE_RunScript("assets/scripts/GameDemo/script_list.lua")
ENGINE_LoadScriptTable(ScriptList)

math.randomseed(os.time())
LoadAssets()

Music.set_volume(20)
SoundFx.set_volume(10,-1)
Music.play("2:23am")

Player_Body = Player:Create(PlayerDefs["body"])
Player_Body_Entity = Entity(Player_Body.m_EntityID)
Player_Glass = LoadEntity(PlayerDefs["glass"])
Player_Glass_Entity =Entity(Player_Glass)
Player_Glass_Entity:set_parent(Player_Body.m_EntityID)

Platform_Mov = Plat:Create(EnvirDefs["platform2"])
Ball_Phys = LoadEntity(EnvirDefs["ball_1"])
Cube_Phys = LoadEntity(EnvirDefs["cube_1"])
Platform_Phys = LoadEntity(EnvirDefs["platform1"])
Ground = LoadEntity(EnvirDefs["ground"])
Wall_1 = LoadEntity(EnvirDefs["wall1"])
Wall_2 = LoadEntity(EnvirDefs["wall2"])
Wall_3 = LoadEntity(EnvirDefs["wall3"])
Wall_4 = LoadEntity(EnvirDefs["wall4"])
Cube_Moveable = LoadEntity(EnvirDefs["cube_3"])
Cube_Trigger = LoadEntity(EnvirDefs["cube_2"])

nanosuit = LoadEntity(ModelDefs["nanosuit"])
backpack = LoadEntity(ModelDefs["backpack"])
cyborg = LoadEntity(ModelDefs["cyborg"])

point_light_1 = LoadEntity(LightDefs["point_light_1"])
point_light_2 = LoadEntity(LightDefs["point_light_2"])
point_light_3 = LoadEntity(LightDefs["point_light_3"])
point_light_4 = LoadEntity(LightDefs["point_light_4"])
direction_light = LoadEntity(LightDefs["direction_light_1"])

local offset = vec3(0.0, 5.0, -10.0)
FollowCam = FollowCamera(Player_Body_Entity, offset)

main = {
	[1] = {
		update = function()
			Player_Body:Update()
			FollowCam:update()
			Platform_Mov:Update(ENGINE_DeltaTime())
			UpdateBullets(ENGINE_DeltaTime())
		end
	},
	[2] = {
		render = function()

		end
	},
}