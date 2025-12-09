GameState = {}
GameState.__index = GameState

function GameState:Create(stack)
	local this = 
	{
		m_Stack = stack,
		
		m_Player = nil,
		m_Glass = nil,
		m_Ball = nil,
		m_Cube = nil,
		m_Trigger = nil,
		m_Floor = nil,
		m_Playtform = nil,
		m_FollowCam = nil,
	}
	local state = state("Game State")
	state:set_variable_table(this)
	state:set_on_enter(function() this:OnEnter() end)
	state:set_on_exit(function() this:OnExit() end)
	state:set_on_update(function(dt) this:OnUpdate(dt) end)
	state:set_on_render(function() this:OnRender() end)
	state:set_handle_inputs(function() this:HandleInputs() end)
	setmetatable(this, self)
	return state
end

function GameState:OnEnter()
	Music.set_volume(20)
	SoundFx.set_volume(10,-1)
	Music.play("2:23am")
	
	--TODO: use or remove the character state machine
	local character = Character:Create({name = "body"})	-- create a character, and activate
	local player_id = character.m_EntityID
	AddActiveCharacter(player_id, character)
	
	self.m_Player = Entity(player_id)
	self.m_Glass = Entity(LoadEntity(PlayerDefs["glass"]))
	self.m_Glass:set_parent(player_id)

	self.m_Ball = Entity(LoadEntity(EnvirDefs["ball_1"]))
	self.m_Cube = Entity(LoadEntity(EnvirDefs["cube_1"]))
	self.m_Cube_Moveable = Entity(LoadEntity(EnvirDefs["cube_3"]))
	self.m_Trigger = Entity(LoadEntity(EnvirDefs["cube_2"]))
	self.m_Floor = Entity(LoadEntity(EnvirDefs["ground"]))
	self.m_Platform = Entity(LoadEntity(EnvirDefs["platform1"]))

	self.speed = 5
	self.m_MovingPlatform = Entity(LoadEntity(EnvirDefs["platform2"]))
	local physics = self.m_MovingPlatform:get_component(Physics)
	physics:set_linear_velocity(vec3(0.0, 0.0, self.speed)) --TODO:add dt

	local offset = vec3(0.0, 5.0, -10.0)
	self.m_FollowCam = FollowCamera(self.m_Player, offset)
end

function GameState:OnExit()
	self.m_Player:kill()
	self.m_Glass:kill()
	self.m_Ball:kill()
	self.m_Cube:kill()
	self.m_Trigger:kill()
	self.m_Floor:kill()
	self.m_Platform:kill()
	self.m_MovingPlatform:kill()
end

function GameState:OnUpdate(dt)
	self.m_FollowCam:update()
	UpdateActiveCharacters(dt)
	UpdateBullets(dt)
	self:UpdateMovingPlatform(dt)
end

function GameState:OnRender()
	-- TODO
end

function GameState:HandleInputs()
	--[[
	if Keyboard.just_pressed(KEY_BACKSPACE) then
		self.m_Stack:pop()
		return
	end
	]]--
end

function GameState:UpdateMovingPlatform(dt)
	local transform = self.m_MovingPlatform:get_component(Transform)
	local physics = self.m_MovingPlatform:get_component(Physics)
	local position = vec3(transform.position)
	if((position.z > 10.0 and self.speed > 0) or (position.z < -10.0 and self.speed < 0))then
		self.speed = -1 * self.speed
		physics:set_linear_velocity(vec3(0.0, 0.0, self.speed))	--TODO:add dt
	end
end