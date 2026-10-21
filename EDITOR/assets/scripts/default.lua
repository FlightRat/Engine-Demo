-- Construction writes editable initial data once. Play uses a separate Lua/Registry.
local function load_definitions()
    ENGINE_RunScript("assets/scripts/GameDemo/script_list.lua")
    ENGINE_LoadScriptTable(ScriptList)
    ENGINE_RunScript("assets/scripts/GameDemo/default_scene_defs.lua")
end

local function require_entity(name)
    return assert(Entity.find(name), "Default scene is missing entity: " .. name)
end

return {
    on_build = function()
        load_definitions()
        local body = LoadEntity(PlayerDefs["body"])
        local bodyEntity = Entity(body)
        local glass = LoadEntity(PlayerDefs["glass"])
        Entity(glass):set_parent(body)

        LoadEntity(EnvirDefs["platform2"])
        LoadEntity(EnvirDefs["ball_1"])
        LoadEntity(EnvirDefs["cube_1"])
        LoadEntity(EnvirDefs["platform1"])
        LoadEntity(EnvirDefs["ground"])
        LoadEntity(EnvirDefs["wall1"])
        LoadEntity(EnvirDefs["wall2"])
        LoadEntity(EnvirDefs["wall3"])
        LoadEntity(EnvirDefs["wall4"])
        LoadEntity(EnvirDefs["cube_2"])

        LoadEntity(ModelDefs["cyborg"])
        LoadEntity(ModelDefs["nanosuit"])
        --owl = LoadEntity(ModelDefs["owl"])
        --backpack = LoadEntity(ModelDefs["backpack"])

        --DisplayPBR()
        DisplayPBR_tex()

        LoadEntity(LightDefs["point_light_1"])
        LoadEntity(LightDefs["point_light_2"])
        LoadEntity(LightDefs["point_light_3"])
        LoadEntity(LightDefs["point_light_4"])
        LoadEntity(LightDefs["direction_light_1"])

        FollowCamera(bodyEntity, vec3(0.0, 5.0, -10.0)):update()
    end,

    on_start = function()
        load_definitions()
        math.randomseed(os.time())
        Music.set_volume(20)
        SoundFx.set_volume(10, -1)
        Music.play("2:23am")

        Player_Body_Entity = require_entity("body")
        Player_Body = Player:Bind(Player_Body_Entity:id())
        Platform_Mov = Plat:Bind(require_entity("platform_2"):id())
        FollowCam = FollowCamera(Player_Body_Entity, vec3(0.0, 5.0, -10.0))
        FollowCam:update()
    end,

    on_update = function(dt)
        Player_Body:Update(dt)
        FollowCam:update()
        Platform_Mov:Update(dt)
        UpdateBullets(dt)
    end,

    on_stop = function()
    end,
}
