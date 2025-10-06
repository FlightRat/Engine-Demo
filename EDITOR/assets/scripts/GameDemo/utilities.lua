
function LoadEntity( def )
	assert(def, "Def does not exitst!")

	local tag = ""
	if def.tag then
		tag = def.tag
	end

	local group = ""
	if def.group then
		group = def.group
	end

	local newEntity = Entity(tag, group)

	if def.components.Transform then
		newEntity:add_component(
			Transform(
				vec3(
					def.components.Transform.position.x,
					def.components.Transform.position.y,
					def.components.Transform.position.z
				),
				vec3(
					def.components.Transform.scale.x,
					def.components.Transform.scale.y,
					def.components.Transform.scale.z
				),
				vec3(
					def.components.Transform.rotation.x,
					def.components.Transform.rotation.y,
					def.components.Transform.rotation.z
				)
			)
		)
	end

	if def.components.Mesh then
		local mesh = newEntity:add_component(
			Mesh(
				def.components.Mesh.type,
				def.components.Mesh.shader,
				vec4(
					def.components.Mesh.color.R,
					def.components.Mesh.color.G,
					def.components.Mesh.color.B,
					def.components.Mesh.color.A
				),
				def.components.Mesh.texture or 0
			)
		)
		mesh.bHidden = def.components.Mesh.bHidden or false
		mesh:load_mesh()
	end

	if def.components.CubeCollider then
		newEntity:add_component(
			CubeCollider(
				def.components.CubeCollider.width,
				def.components.CubeCollider.height
			)
		)
	end

	return newEntity:id()
end

plane_size = 20
function  CheckPos(position, width, height )
	local min_x = -10
	local min_z = -10
	local max_x = 10
	local max_z = 10

	if position.x + width < min_x then
		position.x = position.x + width + plane_size
	elseif position.x > max_x + width then
		position.x = position.x + width - plane_size
	end

	if position.z + width < min_z then
		position.z = position.z + height + plane_size
	elseif position.z > max_z + width  then
		position.z = position.z + height - plane_size
	end
end

function GetRandomPosition()
	return vec3(
		(math.random()*2-1)*10,
		0,
		(math.random()*2-1)*10
	)
end

function GetRandomVelocity(max_speed)
	return vec3(
		(math.random()*2-1)*max_speed,
		0,
		(math.random()*2-1)*max_speed
	)
end

enemy_table = {}
function AddEnemy(enemy)
	table.insert(enemy_table, enemy)
end

function UpdateEnemy()
	for k,v in pairs(enemy_table) do
		v:Update()
	end
end

globalTimer = Timer()
function SpawnEnemy()
	if not globalTimer:is_running() then
		globalTimer:start()
	end

	if globalTimer:elapsed_ms() > 5000 then
		local val = math.random(1, 3)

		if val == 1 then
			local enemy = Enemy:Create("enemy_small")
			AddEnemy(enemy)
		elseif val == 2 then
			local enemy = Enemy:Create("enemy_big")
			AddEnemy(enemy)
		elseif val ==3 then
			-- TODO
		end
		globalTimer:stop()
	end
end

function RemoveEnemy(enemy_id)
	SoundFx.play("hit")	-- put this into branch below if want to play different SoundFx for different enemy
	for k,v in pairs(enemy_table) do
		if v.m_EneityID == enemy_id then
			if v.m_Type == "small" then
				gData:AddScore(SMALL_ENEMY_SCORE)
			elseif v.m_Type == "big" then
				gData:AddScore(BIG_ENEMY_SCORE)
			end
			local enemy = Entity(v.m_EneityID)
			enemy:kill()
			enemy_table[k] = nil
			break
		end
	end
end

function ResetEnemy()
	for k, v in pairs(enemy_table) do
		local enemy = Entity(v.m_EneityID)
		enemy:kill()
		enemy_table[k] = nil
	end
	globalTimer:stop()
end


projectile_table = {}

function AddProjectile(projectile)
	table.insert(projectile_table, projectile)
end

function UpdateProjectile()
	for k,v in pairs(projectile_table) do
		if v:TimesUp() then
			v:Destory()
			projectile_table[k] = nil
		else
			v:Update()
		end
	end
end

function ResetProjectile()
	for k, v in pairs(projectile_table) do
		local projectile = Entity(v.m_EneityID)
		projectile:kill()
		projectile_table[k] = nil
	end
end