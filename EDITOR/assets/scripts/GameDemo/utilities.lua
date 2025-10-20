
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

	if def.components.MeshFilter then
		local meshF = newEntity:add_component(
			MeshFilter(
				def.components.MeshFilter.type
			)
		)
	end

	if def.components.MeshRender then
		local meshR = newEntity:add_component(
			MeshRender(
				def.components.MeshRender.shader,
				vec4(
					def.components.MeshRender.color.R,
					def.components.MeshRender.color.G,
					def.components.MeshRender.color.B,
					def.components.MeshRender.color.A
				),
				def.components.MeshRender.texture or 0
			)
		)
	end

	--TODO: more attributes!!!
	if def.components.Physics then
		local physicsAttr = PhysicsAttributes()
		physicsAttr.position = vec3(
			def.components.Transform.position.x,
			def.components.Transform.position.y,
			def.components.Transform.position.z
		)
		physicsAttr.rotation = vec3(
			def.components.Transform.rotation.x,
			def.components.Transform.rotation.y,
			def.components.Transform.rotation.z
		)
		physicsAttr.type = def.components.Physics.type
		physicsAttr.shape = def.components.Physics.shape
		physicsAttr.box_halfExtents = def.components.Physics.box_halfExtents or vec3(1.0, 1.0, 1.0)
		physicsAttr.sphere_radius = def.components.Physics.sphere_radius or 0.5
		local physics = newEntity:add_component(Physics(physicsAttr))
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