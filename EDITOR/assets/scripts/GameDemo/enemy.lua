Enemy = {}
Enemy.__index = Enemy

function Enemy:Create(def)
	local enemy = EnemyDefs[def]
	assert(enemy, "Enemy def does not exist!")

	local this = 
	{
		m_Def = def,
		--m_Type = enemy.type,
		m_EneityID = -1,
		m_MaxSpeed = enemy.max_speed,
		m_Velocity = GetRandomVelocity(enemy.max_speed)
	}

	this.m_EneityID = LoadEntity(enemy)
	local entity = Entity(this.m_EneityID)
	local transform = entity:get_component(Transform)
	transform.position = GetRandomPosition()

	setmetatable(this, self)
	return this

end

function Enemy:Update()
	local enemy = Entity(self.m_EneityID)
	local transform = enemy:get_component(Transform)

	transform.position.x = transform.position.x + self.m_Velocity.x
	transform.position.z = transform.position.z + self.m_Velocity.z

	CheckPos(transform.position, 0.5, 0.5)

end