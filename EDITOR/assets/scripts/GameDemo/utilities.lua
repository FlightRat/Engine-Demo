
function LoadEntity( def )
	assert(def, "Def does not exitst!")

	local tag = ""
	if def.tag then
		tag = def.tag
	end

	local group = ""
	if def.group then
		group = def.group
	end

	local newEntity = Entity(tag, group)

	if def.components.Transform then
		newEntity:add_component(
			Transform(
				vec3(
					def.components.Transform.position.x,
					def.components.Transform.position.y,
					def.components.Transform.position.z
				),
				vec3(
					def.components.Transform.scale.x,
					def.components.Transform.scale.y,
					def.components.Transform.scale.z
				)
			)
		)
	end

	if def.components.Mesh then
		local mesh = newEntity:add_component(
			Mesh(
				vec3(
					def.components.Mesh.color.R,
					def.components.Mesh.color.G,
					def.components.Mesh.color.B
				),
				def.components.Mesh.type
			)
		)
		mesh:load_mesh()
	end
	return newEntity:id()
end