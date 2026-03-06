Bullets = {}
function AddBullet(bullet)
	Bullets[bullet.m_EntityID] = bullet end
function UpdateBullets(dt)
	for k,v in pairs(Bullets) do
		if v:TimesUp() then
			v:Destroy()
			Bullets[k]=nil
		else
			v:Update(dt)
		end
	end end
function ResetBullets()
	for k,v in pairs(Bullets) do
		v:Destroy()
		Bullets[k]=nil
	end end


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
				def.components.Transform.position,
				def.components.Transform.scale,
				def.components.Transform.rotation
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
		local meshR = newEntity:add_component(MeshRender())
		local materialsData = def.components.MeshRender.material
		for i, mDef in ipairs(materialsData) do
			local material = Material()

			material.shaderName = mDef.shaderName
			material.color = mDef.color
			material.shininess = mDef.shininess
			material.useTex = mDef.useTex
			-- TODO: make this automatic
			material:addTexture("diffuse", mDef.diffuse or "")
			material:addTexture("specular", mDef.specular or "")
		
			meshR:add_material(material)
		end
	end

	if def.components.Light then
		local light = newEntity:add_component(
			Light(
				def.components.Light.diffuse,
				def.components.Light.specular,
				def.components.Light.ambient,

				def.components.Light.type,

				def.components.Light.pos or vec3(0.0, 0.0, 0.0),
				def.components.Light.constant or 1.0,
				def.components.Light.linear or 1.0,
				def.components.Light.quadratic or 1.0,

				def.components.Light.direction or vec3(0.0, 0.0, 0.0)

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
		physicsAttr.position = def.components.Transform.position
		physicsAttr.rotation = def.components.Transform.rotation
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

