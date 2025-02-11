#pragma once

#include "collision_gjk.h"

struct epa_triangle
{
	uint16 a;
	uint16 b;
	uint16 c;

	uint16 edgeOppositeA;
	uint16 edgeOppositeB;
	uint16 edgeOppositeC;

	vec3 normal;
	float distanceToOrigin;
};

struct epa_edge
{
	// Edge points from a to b.
	// The triangles are sorted such that triangleA is on the "left" and triangleB is on the "right", seen from the outside of the simplex.

	uint16 a;
	uint16 b;

	union
	{
		struct
		{
			uint16 triangleA;
			uint16 triangleB;
		};
		uint16 triangles[2];
	};
};

struct epa_triangle_info
{
	vec3 normal;
	float distanceToOrigin;
};

struct epa_simplex
{
	// TODO: Find better array sizes.
	gjk_support_point points[1024];
	epa_triangle triangles[1024];
	epa_edge edges[1024];

	uint32 activeTrianglesMask[1024 / 32];
	uint16 numTriangles;
	uint16 numPoints;
	uint16 numEdges;

	bool isTriangleActive(uint32 index);
	void setTriangleActive(uint32 index);
	void setTriangleInactive(uint32 index);
	uint16 pushPoint(const gjk_support_point& a);
	uint16 pushTriangle(uint16 a, uint16 b, uint16 c, uint16 edgeOppositeA, uint16 edgeOppositeB, uint16 edgeOppositeC, epa_triangle_info info);
	uint16 pushEdge(uint16 a, uint16 b, uint16 triangleA, uint16 triangleB);
	uint32 findTriangleClosestToOrigin();
	bool addNewPointAndUpdate(const gjk_support_point& newPoint);

	static epa_triangle_info getTriangleInfo(const gjk_support_point& a, const gjk_support_point& b, const gjk_support_point& c);
};

struct epa_result
{
	vec3 point;
	vec3 normal;
	float penetrationDepth;
};

enum epa_status
{
	epa_none, // Not returned from EPA. Only for debug output (signals that function was not called).
	epa_success,
	epa_out_of_memory,
	epa_max_num_iterations_reached,
};

static const char* epaReturnNames[] =
{
	"None",
	"Success",
	"Out of memory",
	"Max num iterations reached",
};

// Regardless of the return type, this function always returns the best approximation of the collision info in outResult.
// The caller can decide whether to use the result or not.
template <typename shapeA_t, typename shapeB_t>
NODISCARD static epa_status epaCollisionInfo(const gjk_simplex& gjkSimplex, const shapeA_t& shapeA, const shapeB_t& shapeB, epa_result& outResult, uint32 maxNumIterations = 20)
{
	ASSERT(gjkSimplex.numPoints == 4);

	epa_simplex epaSimplex;
	epaSimplex.numTriangles = 0;
	epaSimplex.numPoints = 0;
	epaSimplex.numEdges = 0;
	memset(epaSimplex.activeTrianglesMask, 0, sizeof(epaSimplex.activeTrianglesMask));

	epaSimplex.pushPoint(gjkSimplex.a);
	epaSimplex.pushPoint(gjkSimplex.b);
	epaSimplex.pushPoint(gjkSimplex.c);
	epaSimplex.pushPoint(gjkSimplex.d);

	epaSimplex.pushTriangle(0, 1, 3, 4, 3, 0, epa_simplex::getTriangleInfo(gjkSimplex.a, gjkSimplex.b, gjkSimplex.d));
	epaSimplex.pushTriangle(1, 2, 3, 5, 4, 1, epa_simplex::getTriangleInfo(gjkSimplex.b, gjkSimplex.c, gjkSimplex.d));
	epaSimplex.pushTriangle(2, 0, 3, 3, 5, 2, epa_simplex::getTriangleInfo(gjkSimplex.c, gjkSimplex.a, gjkSimplex.d));
	epaSimplex.pushTriangle(0, 2, 1, 1, 0, 2, epa_simplex::getTriangleInfo(gjkSimplex.a, gjkSimplex.c, gjkSimplex.b));

	epaSimplex.pushEdge(0, 1, 0, 3);
	epaSimplex.pushEdge(1, 2, 1, 3);
	epaSimplex.pushEdge(2, 0, 2, 3);
	epaSimplex.pushEdge(0, 3, 2, 0);
	epaSimplex.pushEdge(1, 3, 0, 1);
	epaSimplex.pushEdge(2, 3, 1, 2);

	uint32 closestIndex = 0;

	epa_status returnCode = epa_max_num_iterations_reached;

	for (uint32 iteration = 0; iteration < maxNumIterations; ++iteration)
	{
		closestIndex = epaSimplex.findTriangleClosestToOrigin();
		epa_triangle& tri = epaSimplex.triangles[closestIndex];

		gjk_support_point a = support(shapeA, shapeB, tri.normal);

		float d = dot(a.minkowski, tri.normal);
		if (d - tri.distanceToOrigin < 0.01f)
		{
			// Success.
			returnCode = epa_success;
			break;
		}

		if (!epaSimplex.addNewPointAndUpdate(a))
		{
			//std::cout << "EPA out of memory.\n";
			returnCode = epa_out_of_memory;
			break;
		}
	}

	epa_triangle& tri = epaSimplex.triangles[closestIndex];

	gjk_support_point& a = epaSimplex.points[tri.a];
	gjk_support_point& b = epaSimplex.points[tri.b];
	gjk_support_point& c = epaSimplex.points[tri.c];

	vec3 barycentricCoords = getBarycentricCoordinates(a.minkowski, b.minkowski, c.minkowski, tri.normal * tri.distanceToOrigin);
	vec3 pointA = barycentricCoords.x * a.shapeAPoint + barycentricCoords.y * b.shapeAPoint + barycentricCoords.z * c.shapeAPoint;
	vec3 pointB = barycentricCoords.x * a.shapeBPoint + barycentricCoords.y * b.shapeBPoint + barycentricCoords.z * c.shapeBPoint;

	outResult.point = 0.5f * (pointA + pointB);
	outResult.normal = tri.normal;
	outResult.penetrationDepth = tri.distanceToOrigin;
	return returnCode;
}