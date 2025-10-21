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
local floor = LoadEntity(EnvirDefs["floor"])

local ball_entity = Entity(ball)
local cube_entity = Entity(cube)

local cam = Camera:get()
--gFollowCam = FollowCamera(ball_entity)

function controlEntity(entity)
	local physics = entity:get_component(Physics)
	local velocity = physics:get_linear_velocity()
	if Keyboard.just_pressed(KEY_D) then
		physics:set_linear_velocity(vec3(5, velocity.y, velocity.z))
	end
	if Keyboard.just_pressed(KEY_A) then
		physics:set_linear_velocity(vec3(-5, velocity.y, velocity.z))
	end
	if Keyboard.just_pressed(KEY_W) then
		physics:set_linear_velocity(vec3(velocity.x, velocity.y, -5))
	end
	if Keyboard.just_pressed(KEY_S) then
		physics:set_linear_velocity(vec3(velocity.x, velocity.y, 5))
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
		controlEntity(cube_entity)
		controlCamera()
		--gFollowCam:update()
		end
	},
	[2] = {
		render = function()
		end
	},
}