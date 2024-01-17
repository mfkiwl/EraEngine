#pragma once

#ifdef setBit
#undef setBit
#endif // setBit

#include <core/math.h>

#define NDEBUG 0

#define PX_GPU_BROAD_PHASE 1

#define PX_CONTACT_BUFFER_SIZE 64

#define PX_ENABLE_RAYCAST_CCD 0
#define PX_ENABLE_PVD 0

#define PX_PHYSICS_ENABLED = 1

#define PX_NB_MAX_RAYCAST_HITS 64
#define PX_NB_MAX_RAYCAST_DISTANCE 128

#define PX_VEHICLE 0

#define PX_RELEASE(x)	if(x)	{ x->release(); x = nullptr;}
#define UNUSED(x) (void)(x)

#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include "extensions/PxRaycastCCD.h"

#include <px/physics/px_rigidbody_component.h>
#include <set>
#include <unordered_map>
#include <iostream>
#include <core/memory.h>
#include <core/log.h>
#include <queue>

struct application;
struct eallocator;

using namespace physx;

struct px_allocator_callback : PxAllocatorCallback
{
	void* allocate(size_t size, const char* typeName, const char* filename, int line) override
	{
		ASSERT(size < GB(1));
		return _aligned_malloc(size, 16);
	}

	void deallocate(void* ptr) override
	{
		_aligned_free(ptr);
	}
};

struct collision_handling_data
{
	uint32_t id1;
	uint32_t id2;
};

template<typename HitType>
class DynamicHitBuffer : public PxHitCallback<HitType>
{
private:
	uint32 _count;
	HitType _buffer[PX_CONTACT_BUFFER_SIZE];

public:
	DynamicHitBuffer()
		: PxHitCallback<HitType>(_buffer, PX_CONTACT_BUFFER_SIZE)
		, _count(0)
	{
	}

public:
	PX_INLINE PxU32 getNbAnyHits() const
	{
		return getNbTouches();
	}

	PX_INLINE const HitType& getAnyHit(const PxU32 index) const
	{
		PX_ASSERT(index < getNbTouches() + PxU32(this->hasBlock));
		return index < getNbTouches() ? getTouches()[index] : this->block;
	}

	PX_INLINE PxU32 getNbTouches() const
	{
		return _count;
	}

	PX_INLINE const HitType* getTouches() const
	{
		return _buffer;
	}

	PX_INLINE const HitType& getTouch(const PxU32 index) const
	{
		PX_ASSERT(index < getNbTouches());
		return _buffer[index];
	}

	PX_INLINE PxU32 getMaxNbTouches() const
	{
		return PX_CONTACT_BUFFER_SIZE;
	}

protected:
	PxAgain processTouches(const HitType* buffer, PxU32 nbHits) override
	{
		nbHits = min(nbHits, PX_CONTACT_BUFFER_SIZE - _count);
		for (PxU32 i = 0; i < nbHits; i++)
		{
			_buffer[_count + i] = buffer[i];
		}
		_count += nbHits;
		return true;
	}

	void finalizeQuery() override
	{
		if (this->hasBlock)
		{
			processTouches(&this->block, 1);
		}
	}
};

#define PX_SCENE_QUERY_SETUP(blockSingle) \
const PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eMESH_MULTIPLE | PxHitFlag::eUV; \
PxQueryFilterData filterData; \
filterData.flags |= PxQueryFlag::eDYNAMIC | PxQueryFlag::eSTATIC; \
filterData.data.word0 = layerMask; \
filterData.data.word1 = blockSingle ? 1 : 0; \
filterData.data.word2 = hitTriggers ? 1 : 0

#define PX_SCENE_QUERY_SETUP_SWEEP_CAST_ALL() PX_SCENE_QUERY_SETUP(true); \
		PxSweepBufferN<1> buffer

#define PX_SCENE_QUERY_SETUP_SWEEP_CAST() PX_SCENE_QUERY_SETUP(false); \
		DynamicHitBuffer<PxSweepHit> buffer

#define PX_SCENE_QUERY_SETUP_CHECK() PX_SCENE_QUERY_SETUP(false); \
		PxOverlapBufferN<1> buffer

#define PX_SCENE_QUERY_SETUP_OVERLAP() PX_SCENE_QUERY_SETUP(false); \
		DynamicHitBuffer<PxOverlapHit> buffer

#define PX_SCENE_QUERY_COLLECT_OVERLAP() results.clear(); \
		results.resize(buffer.getNbTouches()); \
		size_t resultSize = results.size(); \
		for (int32 i = 0; i < resultSize; i++) \
		{ \
			auto& hitInfo = results[i]; \
			const auto& hit = buffer.getTouch(i); \
			hitInfo = hit.shape ? static_cast<uint32_t*>(hit.shape->userData) : nullptr; \
		}

struct px_raycast_info
{
	px_rigidbody_component* actor = nullptr;

	float distance = 0.0f;
	unsigned int hitCount = 0;
	vec3 position = vec3(0.0f);
};

struct px_overlap_info
{
	bool isOverlapping;
	std::vector<uint32_t*> results;
};

namespace physx
{
	static PxVec3 createPxVec3(const vec3& vec) noexcept { return PxVec3(vec.x, vec.y, vec.z); }
	static PxVec2 createPxVec2(const vec2& vec) noexcept { return PxVec2(vec.x, vec.y); }
	static PxVec3 createPxVec3(vec3&& vec) noexcept { return PxVec3(vec.x, vec.y, vec.z); }
	static PxVec2 createPxVec2(vec2&& vec) noexcept { return PxVec2(vec.x, vec.y); }

	static PxQuat createPxQuat(const quat& q) noexcept { return PxQuat(q.x, q.y, q.z, q.w); }
	static PxQuat createPxQuat(quat&& q) noexcept { return PxQuat(q.x, q.y, q.z, q.w); }

	static vec3 createVec3(const PxVec3& vec) noexcept { return vec3(vec.x, vec.y, vec.z); }
	static vec2 createVec2(const PxVec2& vec) noexcept { return vec2(vec.x, vec.y); }
	static vec3 createVec3(PxVec3&& vec) noexcept { return vec3(vec.x, vec.y, vec.z); }
	static vec2 createVec2(PxVec2&& vec) noexcept { return vec2(vec.x, vec.y); }

	static quat createQuat(const PxQuat& q) noexcept { return quat(q.x, q.y, q.z, q.w); }
	static quat createQuat(PxQuat&& q) noexcept { return quat(q.x, q.y, q.z, q.w); }

	static PxVec2 min(const PxVec2& a, const PxVec2& b) noexcept { return PxVec2(std::min(a.x, b.x), std::min(a.y, b.y)); }
	static PxVec3 min(const PxVec3& a, const PxVec3& b) noexcept { return PxVec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)); }

	static PxVec2 max(const PxVec2& a, const PxVec2& b) noexcept { return PxVec2(std::max(a.x, b.x), std::max(a.y, b.y)); }
	static PxVec3 max(const PxVec3& a, const PxVec3& b) noexcept { return PxVec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)); }
}

struct px_snippet_gpu_load_hook : PxGpuLoadHook
{
	virtual const char* getPhysXGpuDllName() const
	{
		return "PhysXGpu_64.dll";
	}
};

struct px_query_filter : public PxQueryFilterCallback
{
	PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
	{
		if (!shape)
			return PxQueryHitType::eNONE;

		const PxFilterData shapeFilter = shape->getQueryFilterData();
		if ((filterData.word0 & shapeFilter.word0) == 0)
			return PxQueryHitType::eNONE;

		const bool hitTriggers = filterData.word2 != 0;
		if (!hitTriggers && shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
			return PxQueryHitType::eNONE;

		const bool blockSingle = filterData.word1 != 0;
		return blockSingle ? PxQueryHitType::eBLOCK : PxQueryHitType::eTOUCH;
	}

	PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override
	{
		return PxQueryHitType::eNONE;
	}
};

struct px_simulation_filter_callback : PxSimulationFilterCallback
{
	PxFilterFlags pairFound(PxU32 pairID,
		PxFilterObjectAttributes attributes0, PxFilterData filterData0, const PxActor* a0, const PxShape* s0,
		PxFilterObjectAttributes attributes1, PxFilterData filterData1, const PxActor* a1, const PxShape* s1,
		PxPairFlags& pairFlags) override
	{
		return PxFilterFlags(PxFilterFlag::eDEFAULT);
	};

	void pairLost(PxU32 pairID,
		PxFilterObjectAttributes attributes0,
		PxFilterData filterData0,
		PxFilterObjectAttributes attributes1,
		PxFilterData filterData1,
		bool objectRemoved) override
	{

	};

	bool statusChange(PxU32& pairID, PxPairFlags& pairFlags, PxFilterFlags& filterFlags) override
	{
		return false;
	};
};

class px_character_controller_filter_callback : public PxControllerFilterCallback
{
	static PxShape* getShape(const PxController& controller)
	{
		PxRigidDynamic* actor = controller.getActor();

		if (!actor || actor->getNbShapes() < 1)
			return nullptr;

		PxShape* shape = nullptr;
		actor->getShapes(&shape, 1);

		return shape;
	}

	bool filter(const PxController& a, const PxController& b) override
	{
		PxShape* shapeA = getShape(a);
		if (!shapeA)
			return false;

		PxShape* shapeB = getShape(b);
		if (!shapeB)
			return false;

		if (PxFilterObjectIsTrigger(shapeB->getFlags()))
			return false;

		const PxFilterData shapeFilterA = shapeA->getQueryFilterData();
		const PxFilterData shapeFilterB = shapeB->getQueryFilterData();
		if (shapeFilterA.word0 & shapeFilterB.word1)
			return true;

		return false;
	}
};

struct px_profiler_callback : PxProfilerCallback
{
	void* zoneStart(const char* eventName, bool detached, uint64_t contextId) override
	{
		LOG_MESSAGE(eventName);
		return nullptr;
	}

	void zoneEnd(void* profilerData, const char* eventName, bool detached, uint64_t contextId) override
	{
	}
};

struct px_error_reporter : PxErrorCallback
{
	void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override
	{
		if (message)
		{
			LOG_ERROR("PhysX Error! Message: ");
			LOG_ERROR(message, " Code: ", static_cast<int32>(code), " Source: ", file, " : ", line);
		}
		else
			std::cerr << "PhysX Error! \n";
	}
};

struct px_collision_contact_callback : PxSimulationEventCallback
{
	void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override { std::cout << "onConstraintBreak\n"; }
	void onWake(physx::PxActor** actors, physx::PxU32 count) override { std::cout << "onWake\n"; }
	void onSleep(physx::PxActor** actors, physx::PxU32 count) override { std::cout << "onSleep\n"; }
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override { std::cout << "onTrigger\n"; }
	void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override { std::cout << "onAdvance\n"; }
	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
};

struct px_CCD_contact_modification : PxCCDContactModifyCallback
{
	void onCCDContactModify(PxContactModifyPair* const pairs, PxU32 count);
};

class px_physics_engine;

struct px_physics
{
	px_physics() = default;
	~px_physics();

	void initialize();

	void release();

	void releaseScene();

	PxScene* scene = nullptr;

	PxPhysics* physics = nullptr;

	PxCooking* cooking = nullptr;

	PxPvd* pvd = nullptr;

	PxPhysicsInsertionCallback* insertationCallback = nullptr;

	PxDefaultAllocator default_allocator_callback;
	px_allocator_callback allocator_callback;
	px_error_reporter error_reporter;
	px_profiler_callback profiler_callback;

	PxCudaContextManager* cudaContextManager = nullptr;

	PxDefaultErrorCallback defaultErrorCallback;

	RaycastCCDManager* raycastCCD = nullptr;

	PxFoundation* foundation = nullptr;

	px_query_filter queryFilter;

	PxTolerancesScale toleranceScale;
	PxDefaultCpuDispatcher* dispatcher = nullptr;

private:
	bool released = false;

	const uint32_t nbCPUDispatcherThreads = 4;

	friend class px_physics_engine;
};

struct px_triangle_mesh
{
	PxTriangleMesh* createTriangleMesh(PxTriangleMeshDesc desc);
};

#if PX_VEHICLE

struct px_wheel_filter : PxQueryFilterCallback
{
	PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
	{
		if (!shape)
			return PxQueryHitType::eNONE;

		const PxFilterData shapeFilter = shape->getQueryFilterData();

		if (filterData.word3 == shapeFilter.word3)
			return PxQueryHitType::eNONE;

		if ((filterData.word0 & shapeFilter.word1) && (shapeFilter.word0 & filterData.word1))
			return PxQueryHitType::eBLOCK;

		return PxQueryHitType::eNONE;
	}
};

#endif

class px_physics_engine
{
private:
	px_physics_engine(application* application) noexcept;
	~px_physics_engine();

	px_physics_engine(const px_physics_engine&) = delete;
	px_physics_engine& operator=(const px_physics_engine&) = delete;
	px_physics_engine(px_physics_engine&&) = delete;
	px_physics_engine& operator=(px_physics_engine&&) = delete;

public:
	static void initialize(application* application);

	static void release();

	void start();
	void update(float dt);

	void resetActorsVelocityAndInertia();

	void addActor(px_rigidbody_component* actor, PxRigidActor* ractor, bool addToScene);
	void removeActor(px_rigidbody_component* actor);

	static px_physics_engine* get();

	static PxPhysics* getPhysics();

	void releaseActors() noexcept;

	float frameRate = 60.0f;

	px_physics* getPhysicsAdapter() const noexcept { return physics; }

	std::set<px_rigidbody_component*> actors;
	std::unordered_map<PxRigidActor*, px_rigidbody_component*> actors_map;
	static std::queue<collision_handling_data> collisionQueue;

	// Raycasting
	px_raycast_info raycast(px_rigidbody_component* rb, const vec3& dir, int maxDist = PX_NB_MAX_RAYCAST_DISTANCE, bool hitTriggers = true, uint32_t layerMask = 0, int maxHits = PX_NB_MAX_RAYCAST_HITS);

	// Checking
	static bool checkBox(const vec3& center, const vec3& halfExtents, const quat& rotation, bool hitTriggers = false, uint32 layerMask = 0)
	{
		PX_SCENE_QUERY_SETUP_CHECK();
		std::vector<uint32_t*> results;
		const PxTransform pose(createPxVec3(center - vec3(0.0f)), createPxQuat(rotation));
		const PxBoxGeometry geometry(createPxVec3(halfExtents));

		return engine->physics->scene->overlap(geometry, pose, buffer, filterData, &engine->physics->queryFilter);
	}

	static bool checkSphere(const vec3& center, const float radius, bool hitTriggers = false, uint32 layerMask = 0)
	{
		PX_SCENE_QUERY_SETUP_CHECK();
		std::vector<uint32_t*> results;
		const PxTransform pose(createPxVec3(center - vec3(0.0f)));
		const PxSphereGeometry geometry(radius);

		return engine->physics->scene->overlap(geometry, pose, buffer, filterData, &engine->physics->queryFilter);
	}

	static bool checkCapsule(const vec3& center, const float radius, const float halfHeight, const quat& rotation, bool hitTriggers = false, uint32 layerMask = 0)
	{
		PX_SCENE_QUERY_SETUP_CHECK();
		std::vector<uint32_t*> results;
		const PxTransform pose(createPxVec3(center - vec3(0.0f)), createPxQuat(rotation));
		const PxCapsuleGeometry geometry(radius, halfHeight);
		return engine->physics->scene->overlap(geometry, pose, buffer, filterData, &engine->physics->queryFilter);
	}

	// Overlapping
	static px_overlap_info overlapCapsule(const vec3& center, const float radius, const float halfHeight, const quat& rotation, bool hitTriggers = false, uint32 layerMask = 0)
	{
		PX_SCENE_QUERY_SETUP_OVERLAP();
		std::vector<uint32_t*> results;
		const PxTransform pose(createPxVec3(center - vec3(0.0f)), createPxQuat(rotation));
		const PxCapsuleGeometry geometry(radius, halfHeight);
		if (!engine->physics->scene->overlap(geometry, pose, buffer, filterData, &engine->physics->queryFilter))
			return px_overlap_info(false, results);

		PX_SCENE_QUERY_COLLECT_OVERLAP();

		return px_overlap_info(true, results);
	}

	static px_overlap_info overlapBox(const vec3& center, const vec3& halfExtents, const quat& rotation, bool hitTriggers = false, uint32 layerMask = 0)
	{
		PX_SCENE_QUERY_SETUP_OVERLAP();
		std::vector<uint32_t*> results;
		const PxTransform pose(createPxVec3(center - vec3(0.0f)), createPxQuat(rotation));
		const PxBoxGeometry geometry(createPxVec3(halfExtents));
		
		if (!engine->physics->scene->overlap(geometry, pose, buffer, filterData, &engine->physics->queryFilter))
			return px_overlap_info(false, results);

		PX_SCENE_QUERY_COLLECT_OVERLAP();

		return px_overlap_info(true, results);
	}

	static px_overlap_info overlapSphere(const vec3& center, const float radius, bool hitTriggers = false, uint32 layerMask = 0)
	{
		PX_SCENE_QUERY_SETUP_OVERLAP();
		std::vector<uint32_t*> results;
		const PxTransform pose(createPxVec3(center - vec3(0.0f)));
		const PxSphereGeometry geometry(radius);

		if (!engine->physics->scene->overlap(geometry, pose, buffer, filterData, &engine->physics->queryFilter))
			return px_overlap_info(false, results);

		PX_SCENE_QUERY_COLLECT_OVERLAP();

		return px_overlap_info(true, results);
	}

private:
	px_physics* physics = nullptr;

	application* app = nullptr;

	eallocator allocator;

	bool released = false;

	static px_physics_engine* engine;

	static std::mutex sync;

	friend struct px_CCD_contact_modification;
	friend struct px_collision_contact_callback;
	friend struct px_rigidbody_component;
};