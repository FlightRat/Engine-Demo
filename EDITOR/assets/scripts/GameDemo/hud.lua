Hud = {}
Hud.__index = Hud

function Hud:Create()
	local this = 
	{
		m_Lives = {},
		m_CurrentLives  = gData:MaxLives(),
		m_Score = 0,
	}

	-- Create life entities
	for i = 0, gData:MaxLives()-1 do
		local livesID = LoadEntity(HudDefs["lives"])
		local livesEntity = Entity(livesID)
		local transform = livesEntity:get_component(Transform)
		transform.position = vec3(-0.95+0.15*i, 0.95, 0.0)
		table.insert(this.m_Lives, livesID)
	end

	--  Game over
	this.m_bGameOver = false
	this.m_bShowGameOver = false
	this.m_GameOverID = LoadEntity(HudDefs["game_over"])


	setmetatable(this, self)
	return this
end


function Hud:Update()
	if not self.m_bGameOver then
		self:UpdateLives()
		self:UpdateScore()
	elseif not self.m_bShowGameOver then
		local entity = Entity(self.m_GameOverID)
		local mesh = entity:get_component(Mesh)
		mesh.bHidden = false
		self.m_bShowGameOver = true
	end
end

function Hud:UpdateLives()
	local numLives = gData:NumLives()
	if self.m_CurrentLives ~= numLives then
		for k, v in pairs(self.m_Lives) do
			local mesh = Entity(self.m_Lives[k]):get_component(Mesh)
			if k <= gData:NumLives() then
				mesh.bHidden = false
			else
				mesh.bHidden = true
			end
		end
		self.m_CurrentLives = numLives
	end
	if numLives == 0.0 then
		self.m_bGameOver = true
	end
end

function Hud:UpdateScore()
	
	local current_Score = gData:GetScore()
	if current_Score == self.m_Score then
		return
	end
	self:SetScore(current_Score)
end

function Hud:SetScore(current_Score)
	self.m_Score = current_Score
	print("Score: "..current_Score)
end

function Hud:Reset()
	self.m_bGameOver = false
	self.m_bShowGameOver = false
	self:SetScore(0)

	local mesh = Entity(self.m_GameOverID):get_component(Mesh)
	mesh.bHidden = true
end
