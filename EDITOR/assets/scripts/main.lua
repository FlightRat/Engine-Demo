-- Main Lua Scipt!

gEntity = Entity("Test","group")

local transform = gEntity:add_component(
	Transform(0,0,0,0.5,0.5,0.5)
)

local mesh = gEntity:add_component(
	Mesh()
)

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