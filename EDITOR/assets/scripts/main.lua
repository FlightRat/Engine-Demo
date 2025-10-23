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

local player_body = LoadEntity(PlayerDefs["body"])
local player_glass = LoadEntity(PlayerDefs["glass"])
local ball = LoadEntity(ObjDefs["ball"])
local cube = LoadEntity(ObjDefs["cube"])
local floor = LoadEntity(EnvirDefs["floor"])
local platform1 = LoadEntity(EnvirDefs["platform1"])

local player_body_entity = Entity(player_body)
local player_glass_entity = Entity(player_glass)
player_glass_entity:set_parent(player_body)

local cam = Camera:get()
--gFollowCam = FollowCamera(ball_entity)

function controlPlayer_physics()
	local body_physics = player_body_entity:get_component(Physics)
	local body_transform = player_body_entity:get_component(Transform)
	local glass_transform = player_glass_entity:get_component(Transform)
	if Keyboard.pressed(KEY_W) then
		body_physics:linear_impulse(vec3(0.0, 0.0, 100.0))
	end
	if Keyboard.pressed(KEY_A) then
		body_physics:linear_impulse(vec3(100.0, 0.0, 0.0))
	end
	if Keyboard.pressed(KEY_D) then
		body_physics:linear_impulse(vec3(-100.0, 0.0, 0.0))
	end
	if Keyboard.pressed(KEY_S) then
		body_physics:linear_impulse(vec3(0.0, 0.0, -100.0))
	end
	if Keyboard.pressed(KEY_Q) then
		body_physics:angular_impulse(vec3(0.0, 100.0, 0.0))
	end
	if Keyboard.pressed(KEY_E) then
		body_physics:angular_impulse(vec3(0.0,-100.0, 0.0))
	end
	if Keyboard.just_pressed(KEY_SPACE) then
		body_physics:linear_impulse(vec3(0.0, 1000.0, 0.0))
	end
	--glass_transform.position = body_transform.position
	--glass_transform.rotation_quat = body_transform.rotation_quat
end

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


main = {
	[1] = {
		update = function()
		controlPlayer_physics()
		controlCamera()
		--gFollowCam:update()
		end
	},
	[2] = {
		render = function()
		end
	},
}