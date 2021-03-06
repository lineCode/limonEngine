//
// Created by engin on 12.02.2018.
//

#ifndef LIMONENGINE_PHYSICALPLAYER_H
#define LIMONENGINE_PHYSICALPLAYER_H


#include <glm/glm.hpp>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <BulletDynamics/ConstraintSolver/btGeneric6DofSpring2Constraint.h>
#include <btBulletCollisionCommon.h>
#include <vector>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <memory>

#include "Player.h"
#include "../../Options.h"
#include "../../CameraAttachment.h"
#include "../../Utils/GLMConverter.h"
#include "../../GUI/GUIRenderable.h"
#include "../Sound.h"

static const int STEPPING_TEST_COUNT = 5;


class PhysicalPlayer : public Player, public CameraAttachment {

    const glm::vec3 startPosition = glm::vec3(0, 10, 15);
    const float standingHeight = 2.0f;

    glm::vec3 center, up, right;
    glm::quat view;
    btVector3 inputMovementSpeed;
    btVector3 groundFrictionMovementSpeed; //this is for emulating ground friction
    float groundFrictionFactor = 10.0f;
    btVector3 slowDownFactor = btVector3(2.5f, 1.0f, 2.5f);
    btRigidBody *player;
    btGeneric6DofSpring2Constraint *spring;
    float springStandPoint;
    int collisionGroup;
    int collisionMask;

    std::vector<btCollisionWorld::ClosestRayResultCallback> rayCallbackArray;
    btTransform worldTransformHolder;
    bool onAir;
    bool positionSet = false;
    Options *options;
    std::shared_ptr<Sound> currentSound = nullptr;

    bool dirty;

public:
    glm::vec3 getPosition() const {
        return GLMConverter::BltToGLM(player->getCenterOfMassPosition()) + glm::vec3(0.0f, 1.0f, 0.0f);
    }

    void move(moveDirections);

    void rotate(float xPosition, float yPosition, float xChange, float yChange);

    btRigidBody* getRigidBody() {
        return player;
    }

    void registerToPhysicalWorld(btDiscreteDynamicsWorld *world, int collisionGroup, int collisionMask,
                                     const glm::vec3 &worldAABBMin __attribute((unused)), const glm::vec3 &worldAABBMax __attribute((unused))) {
        world->addRigidBody(getRigidBody(), collisionGroup, collisionMask);
        this->collisionGroup = collisionGroup;
        this->collisionMask = collisionMask;
        world->addConstraint(getSpring(worldAABBMin.y));

        for (int i = 0; i < STEPPING_TEST_COUNT; ++i) {
            for (int j = 0; j < STEPPING_TEST_COUNT; ++j) {
                rayCallbackArray[i * STEPPING_TEST_COUNT + j].m_collisionFilterGroup = this->collisionGroup;
                rayCallbackArray[i * STEPPING_TEST_COUNT + j].m_collisionFilterMask = this->collisionMask;
            }
        }
    }

    void processPhysicsWorld(const btDiscreteDynamicsWorld *world);

    bool isDirty() {
        return dirty;//FIXME this always returns true because nothing sets it false;
    }
    void getCameraVariables(glm::vec3& position, glm::vec3 &center, glm::vec3& up, glm::vec3& right) {
        position = GLMConverter::BltToGLM(this->getRigidBody()->getCenterOfMassPosition());
        position.y += 1.0f;//for putting the camera up portion of capsule
        center = this->center;
        up = this->up;
        right = this->right;
    };

    /**
     * This method requires the world, because it raytests for closest object below the camera.
     * This is required because single sided spring constrain automatically attaches to world itself,
     * and we need to calculate an equilibrium point.
     *
     * @param world
     * @return
     */
    btGeneric6DofSpring2Constraint *getSpring(float minY);

    glm::vec3 getLookDirection() const {
        return this->center;
    };

    void getWhereCameraLooks(glm::vec3 &fromPosition, glm::vec3 &lookDirection) const {
        fromPosition = this->getPosition();
        lookDirection = this->center;
    }

    void ownControl(const glm::vec3& position, const glm::vec3 lookDirection) {
        this->center = glm::normalize(lookDirection);
        this->view.w = 0;
        this->view.x = center.x;
        this->view.y = center.y;
        this->view.z = center.z;
        this->right = glm::normalize(glm::cross(center, up));

        btTransform transform = this->player->getCenterOfMassTransform();
        transform.setOrigin(btVector3(position.x, position.y - 1.0f, position.z));// -1 because the capsule is lower by 1 then camera
        this->player->setWorldTransform(transform);
        this->player->getMotionState()->setWorldTransform(transform);
        this->player->activate();

        this->inputMovementSpeed = btVector3(0,0,0);
        this->groundFrictionMovementSpeed = btVector3(0,0,0);

        positionSet = true;
        spring->setEnabled(false);//don't enable until player is not on air
        cursor->setTranslate(glm::vec2(options->getScreenWidth()/2.0f, options->getScreenHeight()/2.0f));
    };

    CameraAttachment* getCameraAttachment() {
        return this;
    }

    PhysicalPlayer(Options *options, GUIRenderable *cursor);

    ~PhysicalPlayer() {
        delete player;
        delete spring;
    }
};


#endif //LIMONENGINE_PHYSICALPLAYER_H
