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
Music.play("2:23am")

-- player
local player_body = LoadEntity(PlayerDefs["body"])
local player_glass = LoadEntity(PlayerDefs["glass"])
local player_glass_entity = Entity(player_glass)
local player_body_entity = Entity(player_body)
player_glass_entity:set_parent(player_body)
local player_physics = player_body_entity:get_component(Physics)
local player_mesh = player_body_entity:get_component(MeshRender)
function controlPlayer_physics()

	-- print player contact detect
	local touching_trigger = false
	local player_data = player_physics:user_data():get_user_data()
	for k,v in pairs(player_data.contactEntities) do
		--print("player is contacting "..v.tag)
		if(v.group == "trigger") then
			touching_trigger=true
		end
	end
	if touching_trigger then
		player_mesh.color = vec4(1.0,0.0,0.0,1.0)
	else
		player_mesh.color = vec4(0.678, 0.847, 1.0, 1.0)
	end


	-- keyboard control
	if Keyboard.pressed(KEY_W) then
		player_physics:linear_impulse(vec3(0.0, 0.0, 100.0))
	end
	if Keyboard.pressed(KEY_A) then
		player_physics:linear_impulse(vec3(100.0, 0.0, 0.0))
	end
	if Keyboard.pressed(KEY_D) then
		player_physics:linear_impulse(vec3(-100.0, 0.0, 0.0))
	end
	if Keyboard.pressed(KEY_S) then
		player_physics:linear_impulse(vec3(0.0, 0.0, -100.0))
	end
	if Keyboard.pressed(KEY_Q) then
		player_physics:angular_impulse(vec3(0.0, 100.0, 0.0))
	end
	if Keyboard.pressed(KEY_E) then
		player_physics:angular_impulse(vec3(0.0,-100.0, 0.0))
	end
	if Keyboard.just_pressed(KEY_SPACE) then
		player_physics:linear_impulse(vec3(0.0, 1000.0, 0.0))
	end
end

-- objects
local ball_1 = LoadEntity(ObjDefs["ball_1"])
local cube_1 = LoadEntity(ObjDefs["cube_1"])
local cube_2 = LoadEntity(ObjDefs["cube_2"])
local floor = LoadEntity(EnvirDefs["floor"])
local platform1 = LoadEntity(EnvirDefs["platform1"])

-- camera
local cam = Camera:get()
local offset = vec3(0.0, 5.0, -10.0)
gFollowCam = FollowCamera(player_body_entity, offset)
function controlCamera()
	if Keyboard.pressed(KEY_UP) then
		cam.process_key(Camera_Movement.Cam_Forward)
	end
	if Keyboard.pressed(KEY_DOWN) then
		cam.process_key(Camera_Movement.Cam_Backward)
	end
	if Keyboard.pressed(KEY_LEFT) then
		cam.process_key(Camera_Movement.Cam_Left)
	end
	if Keyboard.pressed(KEY_RIGHT) then
		cam.process_key(Camera_Movement.Cam_Right)
	end
	cam.process_mouse(Mouse.offset())
	cam.process_scroll(Mouse.wheel_y())
end

-- user data example
--[[
local objectData = ObjectData("tag", "group", true, true, 666)
local userData = UserData.create_user_data(objectData)
local data1 = userData:get_user_data()
print(data1:to_string())
userData:set_user_data(ObjectData("new tag", "new group", false, false, 996))
local data2 = userData:get_user_data()
print(data2:to_string())
]]--

main = {
	[1] = {
		update = function()
		controlPlayer_physics()
		--controlCamera()
		gFollowCam:update()

		-- print all contact pairs
		--local dataPairs = ContactListener.GetUserDataPairs()
		--for i, a, b in pairs(dataPairs) do
			--print(a.tag.." is contacting "..b.tag)
		--end

		end
	},
	[2] = {
		render = function()
		end
	},
}