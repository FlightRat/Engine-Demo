-- Main Lua Scipt!

cubeEntity = Entity("Cube","group")
local cubeTransform = cubeEntity:add_component(Transform(vec3(0.0),vec3(0.5)))
local cubeMesh = cubeEntity:add_component(Mesh(vec3(1.0, 0.0, 0.0),"cube"))
cubeMesh:load_mesh()
--cubeMesh:mesh_color(vec3(1.0, 0.0, 0.0))

planeEntity = Entity("Plane","group")
local planeTransform = planeEntity:add_component(Transform(vec3(0.0), vec3(1.0)))
local planeMesh = planeEntity:add_component(Mesh(vec3(1.0, 1.0, 1.0),"plane"))
planeMesh:load_mesh()
--planeMesh:mesh_color(vec3(1.0, 1.0, 1.0))

local view = Registry.get_entities(Transform)
--view:exclude(Mesh)
view:for_each(
	function (entity)
		print(entity:name())
	end
)

local x_pos = 0.0
local z_pos = 0.0

main = {
	[1] = {
		update = function()
			--local t = cubeEntity:get_component(Transform)
			--print("name:"..t.xx)

			-- Keyboard
			if Keyboard.pressed(KEY_W) then
				z_pos = z_pos - 0.2
			end
			if Keyboard.pressed(KEY_A) then
				x_pos = x_pos - 0.2
			end
			if Keyboard.pressed(KEY_S) then
				z_pos = z_pos + 0.2
			end
			if Keyboard.pressed(KEY_D) then
				x_pos = x_pos + 0.2
			end
			cubeTransform.position.x = x_pos
			cubeTransform.position.z = z_pos

			--Mouse
			if Mouse.moving() then
				--print("Mouse is moving!")
			end
			if Mouse.just_pressed(MOUSE_LEFT) then
				--print("mouse left pressed!")
			end
			if Mouse.pressed(MOUSE_RIGHT) then
				--print("mouse right pressing!")
			end
			local y_offset = Mouse.wheel_y()
			--print(y_offset)
			local mouse_x, mouse_y = Mouse.screen_pos()
			--print("Mouse pos["..mouse_x..", "..mouse_y.."]")


		end
	},
	[2] = {
		render = function()
			
		end
	},
}