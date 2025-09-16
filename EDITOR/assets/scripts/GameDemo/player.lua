Player = {}
Player.__index = Player

function Player:Create(def)
	local this = 
	{
		m_EntityID = def.id,
		m_MoveSpeed = def.move_speed or 0.2
	}
	setmetatable(this, self)
	return this
end

function Player:UpdatePlayer()
	local player = Entity(self.m_EntityID)
	local transform = player:get_component(Transform)
	local mesh = player:get_component(Mesh)

	if Keyboard.pressed(KEY_W) then
		transform.position.z = transform.position.z - self.m_MoveSpeed
	end
	if Keyboard.pressed(KEY_A) then
		transform.position.x = transform.position.x - self.m_MoveSpeed
	end
		if Keyboard.pressed(KEY_S) then
		transform.position.z = transform.position.z + self.m_MoveSpeed
	end
		if Keyboard.pressed(KEY_D) then
		transform.position.x = transform.position.x + self.m_MoveSpeed
	end
end