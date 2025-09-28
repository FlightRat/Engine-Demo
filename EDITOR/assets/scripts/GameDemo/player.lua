Player = {}
Player.__index = Player

function Player:Create(def)
	local this = 
	{
		m_EntityID = def.id,
		m_MoveSpeed = def.move_speed or 0.2,
		m_RotateSpeed = def.rotate_speed or 5,
		m_CoolDown = def.cool_down or 2,

		m_bDead = false,
		m_NumLives = gData:NumLives(),

		m_DeathTimer = Timer(),
		m_InvincibleTimer = Timer(),
		m_CoolDownTimer = Timer()
	}
	setmetatable(this, self)
	return this
end

function Player:Update()
	if self.m_bDead then
		return
	end

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

	-- Cool down example
	if not self.m_CoolDownTimer:is_running() then
		if Keyboard.just_pressed(KEY_F) then
			print("The player used his ability!!!")
			self.m_CoolDownTimer:start()
		end
	elseif self.m_CoolDownTimer:elapsed_sec() >= self.m_CoolDown then
		self.m_CoolDownTimer:stop()
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

	CheckPos(transform.position, transform.scale.x, transform.scale.z)
	self:CheckDeath()
end

function Player:CheckDeath()
	if self.m_NumLives ~= gData:NumLives() then
		self.m_NumLives = gData:NumLives()
		self.m_InvincibleTimer:start()
	end

	if self.m_InvincibleTimer:is_running() then
		local player = Entity(self.m_EntityID)
		local mesh = player:get_component(Mesh)
		local collider = player:get_component(CubeCollider)

		mesh.color = vec4(mesh.color.x, mesh.color.y, mesh.color.z, 0.5)

		if self.m_InvincibleTimer:elapsed_sec() > 3 then
			collider.bColliding = false
			self.m_InvincibleTimer:stop()
			mesh.color = vec4(mesh.color.x, mesh.color.y, mesh.color.z, 1.0)
		end
	end
end
