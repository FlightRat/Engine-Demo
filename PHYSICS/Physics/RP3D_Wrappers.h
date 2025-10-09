#pragma once
#include<reactphysics3d/reactphysics3d.h>
#include<memory>

using namespace reactphysics3d;

namespace PHYSICS {

	struct PhysicsWorldDestroyer
	{
		std::shared_ptr<PhysicsCommon> common;
		PhysicsWorldDestroyer(std::shared_ptr<PhysicsCommon> c):common(c){}
		void operator()(rp3d::PhysicsWorld* world) const;
	};

	struct RigidBodyDestroyer
	{
		std::shared_ptr<PhysicsWorld> world;
		RigidBodyDestroyer(std::shared_ptr<PhysicsWorld> w) : world(w) {}
		void operator()(rp3d::RigidBody* body) const;
	};

	struct ColliderDestroyer
	{
		std::shared_ptr<RigidBody> body;
		ColliderDestroyer(std::shared_ptr<RigidBody> b):body(b){}
		void operator()(rp3d::Collider* collider) const;
	};

	struct BoxCollisionShapeDestroyer
	{
		std::shared_ptr<PhysicsCommon> common;
		BoxCollisionShapeDestroyer(std::shared_ptr<PhysicsCommon> c) :common(c) {}
		void operator()(rp3d::BoxShape* boxShape) const;
	};


	static std::shared_ptr<PhysicsWorld> MakeSharedPhysicsWorld(std::shared_ptr<PhysicsCommon> common)
	{
		PhysicsWorld* rawWorld = common->createPhysicsWorld();
		return std::shared_ptr<PhysicsWorld>(rawWorld, PhysicsWorldDestroyer(common));
	}

	static std::shared_ptr<RigidBody> MakeSharedRigidBody(std::shared_ptr<PhysicsWorld> world, const rp3d::Transform& transform)
	{
		rp3d::RigidBody* rawBody = world->createRigidBody(transform);
		return std::shared_ptr<rp3d::RigidBody>(rawBody, RigidBodyDestroyer(world));
	}

	static std::shared_ptr<Collider> MakeSharedCollider(std::shared_ptr<RigidBody> body, std::shared_ptr<CollisionShape> shape)
	{
		Collider* rawCollider = body->addCollider(shape.get(), Transform::identity());
		return std::shared_ptr<Collider>(rawCollider, ColliderDestroyer(body));
	}

	static std::shared_ptr<BoxShape> MakeSharedBoxCollisionShape(std::shared_ptr<PhysicsCommon> common, const Vector3 extent)
	{
		rp3d::BoxShape* rawBoxShape = common->createBoxShape(extent);
		return std::shared_ptr<rp3d::BoxShape>(rawBoxShape, BoxCollisionShapeDestroyer(common));
	}
}