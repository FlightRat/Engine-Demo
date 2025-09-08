-- Main Lua Scipt!

cubeEntity = Entity("Cube","group")
local cubeTransform = cubeEntity:add_component(Transform(0,0,0,0.5,0.5,0.5))
local cubeMesh = cubeEntity:add_component(Mesh())
cubeMesh:load_mesh("cube")

planeEntity = Entity("Plane","group")
local planeTransform = planeEntity:add_component(Transform(0,0,0,1.0,1.0,1.0))
local planeMesh = planeEntity:add_component(Mesh())
planeMesh:load_mesh("plane")

local view = Registry.get_entities(Transform)
--view:exclude(Mesh)
view:for_each(
	function (entity)
		print(entity:name())
	end
)

main = {
	[1] = {
		update = function()
			--local t = cubeEntity:get_component(Transform)
			--print("name:"..t.xx)
		end
	},
	[2] = {
		render = function()
			
		end
	},
}