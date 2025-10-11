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

--math.randomseed(os.time())

LoadAssets()

--local player = LoadEntity(PlayerDefs["player"])
--local floor = LoadEntity(EnvirDefs["floor"])
--local enemy_s = Enemy:Create("enemy_small")
--local enemy_b = Enemy:Create("enemy_big")
--AddEnemy(enemy_s)
--AddEnemy(enemy_b)

--gCollisionSystem = CollisionSystem:Create()
--gHud = Hud:Create()

--gPlayer = Player:Create({id=player, move_speed=0.2})

--Music.play("2:23am")

--print("Score: "..0)

--floor
local floor = Entity("", "")
local floor_transform = floor:add_component(Transform(vec3(0.0,0.0,0.0),vec3(1.0,1.0,1.0),vec3(0.0,0.0,0.0)))
local floor_mesh = floor:add_component(Mesh("plane","colorShader",vec4(1.0,1.0,1.0,1.0),0))
local floor_physicsAttr = PhysicsAttributes()
floor_physicsAttr.position = floor_transform.position
floor_physicsAttr.rotation = floor_transform.rotation
floor_physicsAttr.type = BodyType.Static
floor_physicsAttr.shape = "box"
floor_physicsAttr.box_halfExtents = vec3(10.0, 0.0005, 10.0)
local floor_physics = floor:add_component(Physics(floor_physicsAttr))

--cube
local cube = Entity("", "")
local cube_transform = cube:add_component(Transform(vec3(0.0,5.0,-5.0),vec3(0.5,0.5,0.5),vec3(0.0,0.0,0.0)))
local cube_mesh = cube:add_component(Mesh("cube","texShader",vec4(0.0,1.0,0.0,1.0),1))
local cube_physicsAttr = PhysicsAttributes()
cube_physicsAttr.position = cube_transform.position
cube_physicsAttr.rotation = cube_transform.rotation
cube_physicsAttr.type = BodyType.Dynamic
cube_physicsAttr.shape = "box"
cube_physicsAttr.box_halfExtents = vec3(0.5, 0.5, 0.5)
local cube_physics = cube:add_component(Physics(cube_physicsAttr))

--sphere
local sphere = Entity("", "")
local sphere_transform = sphere:add_component(Transform(vec3(0.0,5.0,5.0),vec3(0.5,0.5,0.5),vec3(0.0,0.0,0.0)))
local sphere_mesh = sphere:add_component(Mesh("sphere","texShader",vec4(0.0,0.0,1.0,1.0),2))
local sphere_physicsAttr = PhysicsAttributes()
sphere_physicsAttr.position = sphere_transform.position
sphere_physicsAttr.rotation = sphere_transform.rotation
sphere_physicsAttr.type = BodyType.Dynamic
sphere_physicsAttr.shape = "sphere"
sphere_physicsAttr.sphere_radius = 0.5
local sphere_physics = sphere:add_component(Physics(sphere_physicsAttr))

function updateEntity(entity)
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


main = {
	[1] = {
		update = function()
		updateEntity(sphere)
		end
	},
	[2] = {
		render = function()
		end
	},
}