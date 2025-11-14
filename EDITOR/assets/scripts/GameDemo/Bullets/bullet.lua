Bullet = {}
Bullet.__index = Bullet

function Bullet:Create(params)
	local this = 
	{
		m_Def = params.def,
		m_Dir = params.dir,
		m_Position = params.position,
		m_Rotaion = params.rotation,
		m_EntityID = -1,
		m_Lifetime = params.life_time or 500,
		m_LifeTimer = Timer(),
		m_Speed = params.speed or 250,
	}
	local bulletDef = BulletDefs[this.m_Def]
	assert(bulletDef, string.format("Failed to get bullet def for [%s]!",this.m_Def))
	bulletDef.components.Transform.position = this.m_Position
	bulletDef.components.Transform.rotation = this.m_Rotaion
	this.m_EntityID = LoadEntity(bulletDef)
	this.m_LifeTimer:start()
	setmetatable(this,self)
	return this
end

function Bullet:Update(dt)
	local bullet = Entity(self.m_EntityID)
	local physics = bullet:get_component(Physics)
	--TODO: check the contactEntities of the bullet, and do the result
	local velocity = vec3(self.m_Dir.x * self.m_Speed * dt, self.m_Dir.y * self.m_Speed * dt, self.m_Dir.z * self.m_Speed * dt)
	physics:set_linear_velocity(velocity)
end

function Bullet:TimesUp()
	return self.m_LifeTimer:elapsed_ms() >= self.m_Lifetime
end

function Bullet:Destroy()
	Entity(self.m_EntityID):kill()
	ENGINE_Warn("Killed bullet with id: %s",self.m_EntityID)
end