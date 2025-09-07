-- Main Lua Scipt!

cubeEntity = Entity("Cube","group")
local cubeTransform = cubeEntity:add_component(Transform(0,0,0,0.5,0.5,0.5))
local cubeMesh = cubeEntity:add_component(Mesh())
cubeMesh:load_mesh("cube")

planeEntity = Entity("Cube","group")
local planeTransform = planeEntity:add_component(Transform(0,0,0,1.0,1.0,1.0))
local planeMesh = planeEntity:add_component(Mesh())
planeMesh:load_mesh("plane")

main = {
	[1] = {
		update = function()
			print("We are updating with lua!")
		end
	},
	[2] = {
		render = function()
			print("We are rendering witu lua!")
		end
	},
}