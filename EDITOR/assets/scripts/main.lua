-- Main Lua Scipt!

cubeEntity = Entity("Cube","group")
local cubeTransform = cubeEntity:add_component(Transform(vec3(0.0),vec3(0.5)))
local cubeMesh = cubeEntity:add_component(Mesh())
cubeMesh:load_mesh("cube")

planeEntity = Entity("Plane","group")
local planeTransform = planeEntity:add_component(Transform(vec3(0.0), vec3(1.0)))
local planeMesh = planeEntity:add_component(Mesh())
planeMesh:load_mesh("plane")

local view = Registry.get_entities(Transform)
--view:exclude(Mesh)
view:for_each(
	function (entity)
		print(entity:name())
	end
)

local x_pos = 0.0
local move_right = true

main = {
	[1] = {
		update = function()
			--local t = cubeEntity:get_component(Transform)
			--print("name:"..t.xx)
			
			if move_right and x_pos < 10 then
				x_pos = x_pos + 0.2 
			elseif move_right and x_pos >= 10 then
				move_right = false
			end

			if not move_right and x_pos > -10 then
				x_pos = x_pos - 0.2 
			elseif not move_right and x_pos <= -10 then
				move_right = true
			end

			cubeTransform.position.x = x_pos

		end
	},
	[2] = {
		render = function()
			
		end
	},
}