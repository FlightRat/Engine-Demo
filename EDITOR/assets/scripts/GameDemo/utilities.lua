
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
				vec3(
					def.components.Mesh.color.R,
					def.components.Mesh.color.G,
					def.components.Mesh.color.B
				),
				def.components.Mesh.type
			)
		)
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
		position.x = position.x + plane_size + width
	elseif position.x + width > max_x then
		position.x = position.x - plane_size - width
	end

	if position.z + width < min_z then
		position.z = position.z + plane_size + width
	elseif position.z + width > max_z then
		position.z = position.z - plane_size - width
	end

end


function GetRandomPosition()
	return vec3(
		math.random(-10,10),
		0,
		math.random(-10,10)
	)
end

function GetRandomVelocity( min_speed, max_speed )
	return vec3(
		math.random(min_speed,max_speed),
		0,
		math.random(min_speed,max_speed)
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
function SpawnEnemies()
	if not globalTimer:is_running() then
		globalTimer:start()
	end

	if globalTimer:elapsed_sec() > 3 then
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
