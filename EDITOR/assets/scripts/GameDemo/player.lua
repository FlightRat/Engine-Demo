Player = {}
Player.__index = Player

function Player:Create(def)
	local this = 
	{
		m_EntityID = def.id,
		m_MoveSpeed = def.move_speed or 0.2,
		m_RotateSpeed = def.rotate_speed or 5
	}
	setmetatable(this, self)
	return this
end

function Player:Update()
	local player = Entity(self.m_EntityID)
	local transform = player:get_component(Transform)
	local mesh = player:get_component(Mesh)

	if Keyboard.pressed(KEY_Q) then
		transform.rotation.y = transform.rotation.y + self.m_RotateSpeed
	end
	if Keyboard.pressed(KEY_E) then
		transform.rotation.y = transform.rotation.y - self.m_RotateSpeed
	end

	-- move with rotation
	local forward = vec3(
		-math.cos(math.rad(transform.rotation.y)),
		0,
		math.sin(math.rad(transform.rotation.y))
	)
	if Keyboard.pressed(KEY_W) then
		transform.position = transform.position + self.m_MoveSpeed*forward
	end
	if Keyboard.pressed(KEY_S) then
		transform.position = transform.position - self.m_MoveSpeed*forward
	end

	if Keyboard.just_pressed(KEY_SPACE) then
		local bullet = Projectile:Create(
			{
				def = "proj_1",
				dir = forward,
				start_pos = transform.position,
				rotation = transform.rotation
			}
		)
		AddProjectile(bullet)
	end

	CheckPos(transform.position, 0.5, 0.5)

end