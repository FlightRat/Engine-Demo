TitleState = {}
TitleState.__index = TitleState

function TitleState:Create(stack)
	local this = 
	{
		m_Stack = stack,
		m_Title = nil
	}
	local state = state("Title State")
	state:set_variable_table(this)
	state:set_on_enter(function() this:OnEnter() end)
	state:set_on_exit(function() this:OnExit() end)
	state:set_on_update(function() this:OnUpdate() end)
	state:set_on_render(function() this:OnRender() end)
	state:set_handle_inputs(function() this:HandleInputs() end)
	setmetatable(this, self)
	return state
end

function TitleState:OnEnter()
	self.m_Title = Entity(LoadEntity(HudDefs["game_start"]))
end

function TitleState:OnExit()
	self.m_Title:kill()
end

function TitleState:OnUpdate()
end

function TitleState:OnRender()
end

function TitleState:HandleInputs()
	if Keyboard.just_pressed(KEY_ENTER) then
		self.m_Stack:change_state(GameState:Create(self.m_Stack))
		return
	end
end