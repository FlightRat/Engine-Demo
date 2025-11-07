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
	Music.play("2:23am")
	
	-- create a character, and activate
	local character = Character:Create({name = "body"})
	local player_id = character.m_EntityID
	AddActiveCharacter(player_id, character)
	
	self.m_Player = Entity(player_id)
	self.m_Glass = Entity(LoadEntity(PlayerDefs["glass"]))
	self.m_Glass:set_parent(player_id)

	self.m_Ball = Entity(LoadEntity(EnvirDefs["ball_1"]))
	self.m_Cube = Entity(LoadEntity(EnvirDefs["cube_1"]))
	self.m_Trigger = Entity(LoadEntity(EnvirDefs["cube_2"]))
	self.m_Floor = Entity(LoadEntity(EnvirDefs["ground"]))
	self.m_Playtform = Entity(LoadEntity(EnvirDefs["platform1"]))

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
	self.m_Playtform:kill()
end

function GameState:OnUpdate(dt)
	self.m_FollowCam:update()
	UpdateActiveCharacters(dt)
	Control_Physics(self.m_Player)
end

function GameState:OnRender()
	-- TODO
end

function GameState:HandleInputs()
	if Keyboard.just_pressed(KEY_BACKSPACE) then
		self.m_Stack:pop()
		return
	end
end

function Control_Physics(player)

end