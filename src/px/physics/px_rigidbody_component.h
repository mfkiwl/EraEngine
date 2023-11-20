#pragma once
#include <px/core/px_physics_engine.h>

struct scene_entity;

enum class px_rigidbody_type 
{
	Static,
	Dynamic,
	Kinematic
};

enum class px_force_mode 
{
	Impulse,
	Force
};

struct px_rigidbody_component 
{
	px_rigidbody_component() {};
	px_rigidbody_component(scene_entity* entt, px_rigidbody_type rbtype) noexcept;
	~px_rigidbody_component();

	void addForce(vec3 force, px_force_mode mode = px_force_mode::Impulse) noexcept;

	physx::PxRigidActor* getRigidActor() noexcept;

	void setDisableGravity() noexcept;
	void setEnableGravity() noexcept;

	void setMass(unsigned int mass);
	unsigned int getMass() noexcept;

	void setLockRotation(bool x, bool y, bool z);
	void setLockPosition(bool x, bool y, bool z);

	constexpr bool* getLockRotation() noexcept;
	constexpr bool* getLockPosition() noexcept;

	void setLinearVelocity(vec3& velocity);

	vec3 getLinearVelocity();

	void setAngularVelocity(vec3& velocity);

	vec3 getAngularVelocity();

	vec3 getPhysicsPosition();

	void setPhysicsPositionAndRotation(vec3& pos, quat& rot);

	void setAngularDamping(float damping);

	px_rigidbody_type getType() noexcept { return type; }

	void onCollisionEnter(px_rigidbody_component* collision);

	scene_entity* entity = nullptr;

private:
	void createPhysics();
	physx::PxRigidActor* createActor();

private:
	float restriction = 0.6f;

	trs* transform = nullptr;

	unsigned int mass = 1;

	px_rigidbody_type type = px_rigidbody_type::Dynamic;

	float dynamicFriction = 0.5f;
	float staticFriction = 0.5f;
 
 	physx::PxU32 filterGroup = -1;
	physx::PxU32 filterMask = -1;

	bool* rotationLock = new bool[3]{ 0, 0, 0 };
	bool* positionLock = new bool[3]{ 0, 0, 0 };

	physx::PxRigidDynamicLockFlags rotLockNative;
	physx::PxRigidDynamicLockFlags posLockNative;

	bool useGravity = true;

	physx::PxMaterial* material = nullptr;

	physx::PxRigidActor* actor = nullptr;
}; 