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

local ball = LoadEntity(ObjDefs["ball"])
local cube = LoadEntity(ObjDefs["cube"])
local capsule = LoadEntity(ObjDefs["capsule"])
local floor = LoadEntity(EnvirDefs["floor"])
local platform1 = LoadEntity(EnvirDefs["platform1"])

local capsule_entity = Entity(capsule)

local cam = Camera:get()
--gFollowCam = FollowCamera(ball_entity)

function controlEntity(entity)
	local transform = entity:get_component(Transform)
	local physics = entity:get_component(Physics)
	local physics_attr = physics.attributes
	local physics_velocity = physics:get_linear_velocity()
	if Keyboard.pressed(KEY_D) then
		physics:set_linear_velocity(vec3(5, physics_velocity.y, physics_velocity.z))
	end
	if Keyboard.pressed(KEY_A) then
		physics:set_linear_velocity(vec3(-5, physics_velocity.y, physics_velocity.z))
	end
	if Keyboard.pressed(KEY_W) then
		physics:set_linear_velocity(vec3(physics_velocity.x, physics_velocity.y, -5))
	end
	if Keyboard.pressed(KEY_S) then
		physics:set_linear_velocity(vec3(physics_velocity.x, physics_velocity.y, 5))
	end
	if Keyboard.just_pressed(KEY_Q) then
		physics:set_angular_velocity(vec3(0,5,0))
		print(transform.rotation.y)
	end
	if Keyboard.just_pressed(KEY_E) then
		physics:set_angular_velocity(vec3(0,-5,0))
		print(transform.rotation.y)
	end
	if Keyboard.just_pressed(KEY_SPACE) then
		physics:linear_impulse(vec3(0.0, 1000.0, 0.0))
	end
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
		controlEntity(capsule_entity)
		controlCamera()
		--gFollowCam:update()
		end
	},
	[2] = {
		render = function()
		end
	},
}