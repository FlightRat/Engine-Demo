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
		m_Speed = params.speed or 1000,
	}
	local bulletDef = BulletDefs[this.m_Def]
	assert(bulletDef, string.format("Failed to get bullet def for [%s]!",this.m_Def))
	bulletDef.components.Transform.position = vec3(this.m_Position)
	bulletDef.components.Transform.rotation = vec3(this.m_Rotaion)
	this.m_EntityID = LoadEntity(bulletDef)
	this.m_LifeTimer:start()
	setmetatable(this,self)
	return this
end

function Bullet:Update(dt)
	local bullet = Entity(self.m_EntityID)
	local physics = bullet:get_component(Physics)
	physics:set_linear_velocity(vec3(self.m_Dir * self.m_Speed * dt, 0))
end

function Bullet:TimesUp()
	return self.m_LifeTimer:elapsed_ms() >= self.m_Lifetime
end

function Bullet:Destroy()
	Entity(self.m_EntityID):kill()
	ENGINE_WARN("Killed bullet with id: %s",self.m_EntityID)
end