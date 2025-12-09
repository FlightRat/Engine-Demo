Plat = {}
Plat.__index = Plat

function Plat:Create(def)
	local this = 
	{
		m_EntityID = -1,
		m_speed = 5,
	}
	this.m_EntityID = LoadEntity(def)
	local entity = Entity(this.m_EntityID)
	local physics = entity:get_component(Physics)
	physics:set_linear_velocity(vec3(0.0, 0.0, this.m_speed))

	setmetatable(this,self)
	return this
end

function Plat:Update(dt)
	local entity = Entity(self.m_EntityID)
	local transform = entity:get_component(Transform)
	local physics = entity:get_component(Physics)
	local position = vec3(transform.position)
	if((position.z > 10.0 and self.m_speed > 0) or (position.z < -10.0 and self.m_speed < 0))then
		self.m_speed = -1 * self.m_speed
		physics:set_linear_velocity(vec3(0.0, 0.0, self.m_speed))
	end
end