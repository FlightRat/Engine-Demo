
ActiveCharacters = {}

function AddActiveCharacter(entity_id, character)
	ActiveCharacters[entity_id] = character
end

function GetActiveCharacter(entity_id)
	assert(ActiveCharacters[entity_id], string.format("Character with ID [&d] does not exist", entity_id))
	return ActiveCharacters[entity_id]
end

function ClearCharacters()
	for k,v in pairs(ActiveCharacters) do
		ActiveCharacters[k] = nil
	end
end

function UpdateActiveCharacters(dt)
	for _,v in pairs(ActiveCharacters) do
		v.m_Controller:update(dt)
	end
end

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
				),
				vec3(
					def.components.Transform.rotation.x,
					def.components.Transform.rotation.y,
					def.components.Transform.rotation.z
				)
			)
		)
	end

	if def.components.MeshFilter then
		local meshF = newEntity:add_component(
			MeshFilter(
				def.components.MeshFilter.type
			)
		)
	end

	if def.components.MeshRender then
		local meshR = newEntity:add_component(
			MeshRender(
				def.components.MeshRender.shader,
				def.components.MeshRender.texture,
				vec4(
					def.components.MeshRender.color.R,
					def.components.MeshRender.color.G,
					def.components.MeshRender.color.B,
					def.components.MeshRender.color.A
				)
			)
		)
	end

	--TODO do the default in C++
	if def.components.Physics then
	--[[
		attr_table = def.components.Physics
		position = {x=def.components.Transform.position.x, y=def.components.Transform.position.y, z=def.components.Transform.position.z}
		rotation = {x=def.components.Transform.rotation.x, y=def.components.Transform.rotation.y, z=def.components.Transform.rotation.z}
		table.insert(attr_table, position)
		table.insert(attr_table, rotation)
		local physicsAttr = PhysicsAttributes(attr_table)
	]]--
		local physicsAttr = PhysicsAttributes()
		physicsAttr.position = vec3(
			def.components.Transform.position.x,
			def.components.Transform.position.y,
			def.components.Transform.position.z)
		physicsAttr.rotation = vec3(
			def.components.Transform.rotation.x,
			def.components.Transform.rotation.y,
			def.components.Transform.rotation.z)
		-- rigidbody
		physicsAttr.type = def.components.Physics.type
		-- collider
		physicsAttr.trigger = def.components.Physics.b_Trigger or false
		physicsAttr.shape = def.components.Physics.shape
		physicsAttr.box_halfExtents = def.components.Physics.box_halfExtents or vec3(1.0, 1.0, 1.0)
		physicsAttr.sphere_radius = def.components.Physics.sphere_radius or 1.0
		physicsAttr.capsule_radius = def.components.Physics.capsule_radius or 1.0
		physicsAttr.capsule_halfHeight = def.components.Physics.capsule_halfHeight or 1.0
		-- basic physics attribute
		physicsAttr.enable_gravity = def.components.Physics.enable_gravity or true
		physicsAttr.mass = def.components.Physics.mass or 1.0
		physicsAttr.mass_density = def.components.Physics.mass_density or 1.0
		physicsAttr.bounciness = def.components.Physics.bounciness or 0.5
		physicsAttr.friction = def.components.Physics.friction or 0.3
		-- linear/angular
		physicsAttr.linear_damping = def.components.Physics.linear_damping or 0.0
		physicsAttr.angular_damping = def.components.Physics.angular_damping or 0.0
		physicsAttr.linear_axis_factor = def.components.Physics.linear_axis_factor or vec3(1.0, 1.0, 1.0)
		physicsAttr.angular_axis_factor = def.components.Physics.angular_axis_factor or vec3(1.0, 1.0, 1.0)
		-- objectData
		physicsAttr.objectData=ObjectData(
			tag,group, 
			def.components.Physics.b_Collider or true, 
			def.components.Physics.b_Trigger or false, 
			newEntity:id())
		local physics = newEntity:add_component(Physics(physicsAttr))
	end

	return newEntity:id()
end
