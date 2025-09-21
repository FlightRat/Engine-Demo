CollisionSystem = {}
CollisionSystem.__index = CollisionSystem

function CollisionSystem:Create()
	local this = 
	{
		--TODO:Add more variables as needed
	}
	setmetatable(this, self)
	return this
end

function CollisionSystem:Update()
	self:UpdateCubeCollision()
	--self:UpdateSphereCollision()
end

function CollisionSystem:UpdateCubeCollision()
	local entities = Registry.get_entities(CubeCollider)
	entities:for_each(
		function(entity_a)
			local group_a = entity_a:group()
			local name_a = entity_a:name()
			local collider_a = entity_a:get_component(CubeCollider)

			entities:for_each(
				function(entity_b)
					if entity_a:id() == entity_b:id() then
						goto continue
					end

					local name_b = entity_b:name()
					local group_b = entity_b:group()
					local collider_b = entity_b:get_component(CubeCollider)

					if group_a == group_b then 
						goto continue
					end

					if collider_a.bColliding or collider_b.bColliding then
						goto continue
					end
					
					if self:Intersect_Cube(entity_a, entity_b) then
						print("ID: " ..entity_a:id().. " is colliding with ID: "..entity_b:id())
					end

					::continue::
				end
			)

		end

	)

end

function CollisionSystem:Intersect_Cube(entity_a, entity_b)

	collider_a = entity_a:get_component(CubeCollider)
	collider_b = entity_b:get_component(CubeCollider)

	transform_a = entity_a:get_component(Transform)
	transform_b = entity_b:get_component(Transform)

	local position_a = transform_a.position
	local position_b = transform_b.position

	left_a = position_a.x - collider_a.width
	right_a = position_a.x + collider_a.width
	top_a = position_a.z - collider_a.width
	bottom_a = position_a.z + collider_a.width

	left_b = position_b.x - collider_b.width
	right_b = position_b.x + collider_b.width
	top_b = position_b.z - collider_b.width
	bottom_b = position_b.z + collider_b.width



	if (right_a <= left_b or left_a >= right_b or bottom_a <= top_b or top_a >= bottom_b) then
		return false
	else
		return true
	end
end
