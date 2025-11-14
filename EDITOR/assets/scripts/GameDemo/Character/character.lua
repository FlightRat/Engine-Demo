Character = {}
Character.__index = Character

--[[
一个角色有一个实体ID和一个状态机，一个状态机有很多自己定义的状态，每次更新都是在调用每个角色的当前状态的update函数
player character is created when enter "GameState" -- "Character:Create({name = "body"})"
Character:Create:
	1.load the capsule entity and storage ID
	2.find the table like "PlayerStates" which is customized for the character
	3.load all the states according to "controller" in defs, and push them into stateMachine
	4.change to the default state
the update in GameState in actually updating the currentState of all characters 
]]--

function Character:Create(def)
	local this = 
	{
		m_Def = def,
		m_Group = def.group,
		m_Type = def.type or "PLAYER",
		m_EntityID = -1,
		m_bInitialized = false,
		m_bDead = false,
		m_bRunning = false,
		m_bJumping = false,
		m_bGrounded = true,
		m_Controller = StateMachine(),
	}

	local entityDef = nil
	if this.m_Type == "PLAYER" then
		entityDef = PlayerDefs[def.name]
	elseif this.m_Type == "ENEMY" then
		entityDef = EnemyDefs[def.name]
	end

	assert(entityDef, string.format("Failed to get the entity def for [%s]", def.name))
	this.m_EntityID = LoadEntity(entityDef)

	assert(this.m_EntityID ~= -1, "Failed to load entity")
	assert(this.m_EntityID, "Failed to load entity")

	if entityDef.controller then
		self:InitStateMachine(entityDef, this)
	end

	setmetatable(this,self)
	return this
end

function Character:InitStateMachine(entityDef, character)
	if character.m_bInitialized then
		return
	end

	-- take all pre-defined states
	local states = nil
	if character.m_Type == "PLAYER" then
		assert(PlayerStates, "PlayerStates does not exist")
		states = PlayerStates
	elseif character.m_Type == "ENEMY" then
		assert(EnemyStates, "EnemyStates dose not exist")
		states = EnemyStates
	end

	-- instance states and complete the machine
	for _, name in pairs(entityDef.controller) do
		local state = states[name]
		assert(state, string.format("State [%s] does not exist.", name))
		local instance = state:Create(character)
		character.m_Controller:add_state(instance)
	end

	character.m_Controller:change_state(entityDef.default_state, false,nil)
	character.m_bInitialized=true
end