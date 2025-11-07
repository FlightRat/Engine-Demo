MoveState = {}
MoveState.__index = MoveState

function MoveState:Create(character)
	assert(character)
	local this = 
	{
		m_Character = character,
		m_EntityID = character.m_EntityID,
		m_Controller = character.m_Controller,
		m_JumpSteps = 10,
		m_NumJumps = 0,
		m_bAttacking = false
	}
	local state = state("move")
	state:set_variable_table(this)
	state:set_on_enter(function(...) this:OnEnter(...) end)
	state:set_on_exit(function() this:OnExit() end)
	state:set_on_update(function(dt) this:OnUpdate(dt) end)
	state:set_on_render(function() end)

	setmetatable(this, self)
	return state
end

function MoveState:OnEnter(params) end
function MoveState:OnExit() end
function MoveState:OnUpdate(dt) 
	local player = Entity(self.m_EntityID)
	local mesh_render = player:get_component(MeshRender)
	local physics = player:get_component(Physics)

	-- print player contact detect
	local touching_trigger = false
	local player_data = physics:user_data():get_user_data()
	for k,v in pairs(player_data.contactEntities) do
		--print("player is contacting "..v.tag)
		if(v.group == "trigger") then
			touching_trigger=true
		end
	end
	if touching_trigger then
		mesh_render.color = vec4(1.0,0.0,0.0,1.0)
	else
		mesh_render.color = vec4(0.678, 0.847, 1.0, 1.0)
	end

	-- keyboard control
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
		physics:linear_impulse(vec3(0.0, 1000.0, 0.0))
	end
end