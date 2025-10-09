#include "RP3D_Wrappers.h"
#include<Logger.h>

namespace PHYSICS{
    void PhysicsWorldDestroyer::operator()(rp3d::PhysicsWorld* world) const
    {
        if (common && world)
        {
            //ENGINE_LOG("Physics world destroyed!");
            common->destroyPhysicsWorld(world);
        }
    }

    void RigidBodyDestroyer::operator()(rp3d::RigidBody* body) const
    {
        if (world && body) {
            //ENGINE_LOG("RigidBody destroyed!");
            world->destroyRigidBody(body);
        }
    }

    void BoxCollisionShapeDestroyer::operator()(rp3d::BoxShape* boxShape) const
    {
        if (common && boxShape)
        {   //ENGINE_LOG("boxShape destroyed!");
            common->destroyBoxShape(boxShape);
        }
    }

    void ColliderDestroyer::operator()(rp3d::Collider* collider) const
    {
        if (body && collider)
        {
            //ENGINE_LOG("collider destroyed!");
            body->removeCollider(collider);
        }
    }

}




