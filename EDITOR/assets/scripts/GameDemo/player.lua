Player = {}
Player.__index = Player

function Player:Create(def)
	local this = 
	{
		m_EntityID = -1,
	}
	this.m_EntityID = LoadEntity(def)
	setmetatable(this,self)
	return this
end

function Player:Update(dt)
	local player = Entity(self.m_EntityID)
	local mesh_render = player:get_component(MeshRender)
	local physics = player:get_component(Physics)

	-- print player contact detect
	local touching_trigger = false
	local player_data = physics:user_data():get_user_data()
	for k,v in pairs(player_data.contactEntities) do
		local other = Entity(v.entityID)
		local other_physics = other:get_component(Physics)
		touching_trigger = other_physics.attributes.trigger
	end
	if touching_trigger then
		mesh_render:set_color(vec4(1.0,0.0,0.0,1.0))
	else
		mesh_render:set_color(vec4(0.678, 0.847, 1.0, 1.0))
	end

	-- keyboard control
	--NOTE: if want to move with "linear_velocity", need to keep a "forward", rotate it with QE, and multiply it with speed
	if Keyboard.pressed(KEY_W) then
		physics:linear_impulse(vec3(0.0, 0.0, 100.0))
	end
	if Keyboard.pressed(KEY_A) then
		physics:linear_impulse(vec3(100.0, 0.0, 0.0))
	end
	if Keyboard.pressed(KEY_D) then
		physics:linear_impulse(vec3(-100.0, 0.0, 0.0))
	end
	if Keyboard.pressed(KEY_S) then
		physics:linear_impulse(vec3(0.0, 0.0, -100.0))
	end
	if Keyboard.pressed(KEY_Q) then
		physics:angular_impulse(vec3(0.0, 100.0, 0.0))
	end
	if Keyboard.pressed(KEY_E) then
		physics:angular_impulse(vec3(0.0,-100.0, 0.0))
	end
	if Keyboard.just_pressed(KEY_SPACE) then
		SoundFx.play("jump")
		physics:linear_impulse(vec3(0.0, 1000.0, 0.0))
	end
	if Keyboard.just_pressed(KEY_F) then
		SoundFx.play("shot")
		self:shot()
	end
end

function Player:shot()
	local player = Entity(self.m_EntityID)
	local transform = player:get_component(Transform)

	local bullet = Bullet:Create(
		{
			def = "normal_shot",
			dir = vec3(transform.forward),	-- pass as independent copy
			position = vec3(transform.position),
			rotation = vec3(transform.rotation_eular),
			speed = 1000,
			life_time = 1000
		}
	)
	AddBullet(bullet)
end
