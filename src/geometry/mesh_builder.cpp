#include "pch.h"
#include "mesh_builder.h"
#include "core/color.h"
#include "asset/model_asset.h"

struct vertex_info
{
	uint32 othersSize;
	uint32 skinOffset;
};

static vertex_info getVertexInfo(uint32 flags)
{
	vertex_info result = {};
	if (flags & mesh_creation_flags_with_uvs) { result.othersSize += sizeof(vec2); }
	if (flags & mesh_creation_flags_with_normals) { result.othersSize += sizeof(vec3); }
	if (flags & mesh_creation_flags_with_tangents) { result.othersSize += sizeof(vec3); }
	if (flags & mesh_creation_flags_with_skin) { result.skinOffset = result.othersSize;	result.othersSize += sizeof(skinning_weights); }
	if (flags & mesh_creation_flags_with_colors) { result.othersSize += sizeof(uint32); }
	return result;
}

mesh_builder::mesh_builder(uint32 vertexFlags, mesh_index_type indexType)
{
	positionArena.initialize(0, GB(2));
	othersArena.initialize(0, GB(2));
	indexArena.initialize(0, GB(2));

	this->vertexFlags = vertexFlags;
	this->indexType = indexType;
	indexSize = (indexType == mesh_index_uint16) ? sizeof(uint16) : sizeof(uint32);

	vertex_info info = getVertexInfo(vertexFlags);
	othersSize = info.othersSize;
	skinOffset = info.skinOffset;
}

mesh_builder::~mesh_builder()
{
}

#define pushVertex(pos, uv, nor, tan, skin, col) \
	if (vertexFlags & mesh_creation_flags_with_positions) { *positionPtr++ = pos; }														\
	if (vertexFlags & mesh_creation_flags_with_uvs) { *(vec2*)othersPtr = uv; othersPtr += sizeof(vec2); }								\
	if (vertexFlags & mesh_creation_flags_with_normals) { *(vec3*)othersPtr = nor; othersPtr += sizeof(vec3); }							\
	if (vertexFlags & mesh_creation_flags_with_tangents) { *(vec3*)othersPtr = tan; othersPtr += sizeof(vec3); }						\
	if (vertexFlags & mesh_creation_flags_with_skin) { *(skinning_weights*)othersPtr = skin; othersPtr += sizeof(skinning_weights); }	\
	if (vertexFlags & mesh_creation_flags_with_colors) { *(uint32*)othersPtr = col; othersPtr += sizeof(uint32); }

#define triangle(triangle_type, index_type, a, b, c) *(triangle_type*)indexPtr = flipWindingOrder \
	? triangle_type{ (index_type)((a) + indexOffset), (index_type)((c) + indexOffset), (index_type)((b) + indexOffset) } \
	: triangle_type{ (index_type)((a) + indexOffset), (index_type)((b) + indexOffset), (index_type)((c) + indexOffset) }; \
	indexPtr += sizeof(triangle_type);

#define pushTriangle(a, b, c) \
	if (indexType == mesh_index_uint16) { triangle(indexed_triangle16, uint16, (a), (b), (c)); } \
	else { triangle(indexed_triangle32, uint32, (a), (b), (c)); }

void mesh_builder::pushQuad(const quad_mesh_desc& desc, bool flipWindingOrder)
{
	vec3 center = desc.center;
	vec2 radius = desc.radius;
	quat rotation = desc.rotation;

	auto [positionPtr, othersPtr, indexPtr, indexOffset] = beginPrimitive(4, 2);

	vec3 xAxis = rotation * vec3(1.f, 0.f, 0.f);
	vec3 yAxis = rotation * vec3(0.f, 1.f, 0.f);
	vec3 zAxis = rotation * vec3(0.f, 0.f, 1.f);

	vec3 x = xAxis * radius.x;
	vec3 y = yAxis * radius.y;

	pushVertex(center - x - y, vec2(0.f, 0.f), zAxis, yAxis, {}, 0);
	pushVertex(center + x - y, vec2(1.f, 0.f), zAxis, yAxis, {}, 0);
	pushVertex(center - x + y, vec2(0.f, 1.f), zAxis, yAxis, {}, 0);
	pushVertex(center + x + y, vec2(1.f, 1.f), zAxis, yAxis, {}, 0);

	pushTriangle(0, 1, 2);
	pushTriangle(1, 3, 2);
}

void mesh_builder::pushBox(const box_mesh_desc& desc, bool flipWindingOrder)
{
	vec3 center = desc.center;
	vec3 radius = desc.radius;
	quat rotation = desc.rotation;

	vec3 xAxis = rotation * vec3(1.f, 0.f, 0.f);
	vec3 yAxis = rotation * vec3(0.f, 1.f, 0.f);
	vec3 zAxis = rotation * vec3(0.f, 0.f, 1.f);

	vec3 x = xAxis * radius.x;
	vec3 y = yAxis * radius.y;
	vec3 z = zAxis * radius.z;

	if ((vertexFlags & mesh_creation_flags_with_positions)
		&& !(vertexFlags & mesh_creation_flags_with_uvs)
		&& !(vertexFlags & mesh_creation_flags_with_normals)
		&& !(vertexFlags & mesh_creation_flags_with_tangents))
	{
		auto [positionPtr, othersPtr, indexPtr, indexOffset] = beginPrimitive(8, 12);

		pushVertex(center - x - y + z, {}, {}, {}, {}, 0); // 0
		pushVertex(center + x - y + z, {}, {}, {}, {}, 0);	// x
		pushVertex(center - x + y + z, {}, {}, {}, {}, 0); // y
		pushVertex(center + x + y + z, {}, {}, {}, {}, 0);	// xy
		pushVertex(center - x - y - z, {}, {}, {}, {}, 0); // z
		pushVertex(center + x - y - z, {}, {}, {}, {}, 0); // xz
		pushVertex(center - x + y - z, {}, {}, {}, {}, 0); // yz
		pushVertex(center + x + y - z, {}, {}, {}, {}, 0); // xyz

		pushTriangle(0, 1, 2);
		pushTriangle(1, 3, 2);
		pushTriangle(1, 5, 3);
		pushTriangle(5, 7, 3);
		pushTriangle(5, 4, 7);
		pushTriangle(4, 6, 7);
		pushTriangle(4, 0, 6);
		pushTriangle(0, 2, 6);
		pushTriangle(2, 3, 6);
		pushTriangle(3, 7, 6);
		pushTriangle(4, 5, 0);
		pushTriangle(5, 1, 0);
	}
	else
	{
		auto [positionPtr, othersPtr, indexPtr, indexOffset] = beginPrimitive(24, 12);

		pushVertex(center - x - y + z, vec2(0.f, 0.f), zAxis, yAxis, {}, 0);
		pushVertex(center + x - y + z, vec2(1.f, 0.f), zAxis, yAxis, {}, 0);
		pushVertex(center - x + y + z, vec2(0.f, 1.f), zAxis, yAxis, {}, 0);
		pushVertex(center + x + y + z, vec2(1.f, 1.f), zAxis, yAxis, {}, 0);
		pushVertex(center + x - y + z, vec2(0.f, 0.f), xAxis, yAxis, {}, 0);
		pushVertex(center + x - y - z, vec2(1.f, 0.f), xAxis, yAxis, {}, 0);
		pushVertex(center + x + y + z, vec2(0.f, 1.f), xAxis, yAxis, {}, 0);
		pushVertex(center + x + y - z, vec2(1.f, 1.f), xAxis, yAxis, {}, 0);
		pushVertex(center + x - y - z, vec2(0.f, 0.f), -zAxis, yAxis, {}, 0);
		pushVertex(center - x - y - z, vec2(1.f, 0.f), -zAxis, yAxis, {}, 0);
		pushVertex(center + x + y - z, vec2(0.f, 1.f), -zAxis, yAxis, {}, 0);
		pushVertex(center - x + y - z, vec2(1.f, 1.f), -zAxis, yAxis, {}, 0);
		pushVertex(center - x - y - z, vec2(0.f, 0.f), -xAxis, yAxis, {}, 0);
		pushVertex(center - x - y + z, vec2(1.f, 0.f), -xAxis, yAxis, {}, 0);
		pushVertex(center - x + y - z, vec2(0.f, 1.f), -xAxis, yAxis, {}, 0);
		pushVertex(center - x + y + z, vec2(1.f, 1.f), -xAxis, yAxis, {}, 0);
		pushVertex(center - x + y + z, vec2(0.f, 0.f), yAxis, xAxis, {}, 0);
		pushVertex(center + x + y + z, vec2(1.f, 0.f), yAxis, xAxis, {}, 0);
		pushVertex(center - x + y - z, vec2(0.f, 1.f), yAxis, xAxis, {}, 0);
		pushVertex(center + x + y - z, vec2(1.f, 1.f), yAxis, xAxis, {}, 0);
		pushVertex(center - x - y - z, vec2(0.f, 0.f), -yAxis, xAxis, {}, 0);
		pushVertex(center + x - y - z, vec2(1.f, 0.f), -yAxis, xAxis, {}, 0);
		pushVertex(center - x - y + z, vec2(0.f, 1.f), -yAxis, xAxis, {}, 0);
		pushVertex(center + x - y + z, vec2(1.f, 1.f), -yAxis, xAxis, {}, 0);

		pushTriangle(0, 1, 2);
		pushTriangle(1, 3, 2);
		pushTriangle(4, 5, 6);
		pushTriangle(5, 7, 6);
		pushTriangle(8, 9, 10);
		pushTriangle(9, 11, 10);
		pushTriangle(12, 13, 14);
		pushTriangle(13, 15, 14);
		pushTriangle(16, 17, 18);
		pushTriangle(17, 19, 18);
		pushTriangle(20, 21, 22);
		pushTriangle(21, 23, 22);
	}
}

void mesh_builder::pushTesselatedBox(const tesselated_box_mesh_desc& desc, bool flipWindingOrder)
{
	const uint32 numVerticesPerEdge = desc.numIntervals + 1;
	const uint32 numVertices = 6 * numVerticesPerEdge * numVerticesPerEdge - 12 * numVerticesPerEdge + 8;
	const uint32 numTriangles = desc.numIntervals * desc.numIntervals * 2 * 6;

	auto [positionPtr, othersPtr, indexPtr, indexOffset] = beginPrimitive(numVertices, numTriangles);

	float distanceBetweenVertices = 1.f / desc.numIntervals;

	uint32 vertexIndex = 0;
	uint32 triangleIndex = 0;

	vec3 center = desc.center;
	quat rotation = desc.rotation;
	vec3 radius = desc.radius;

	// Upper face
	for (uint32 z = 0; z < numVerticesPerEdge; ++z)
	{
		for (uint32 x = 0; x < numVerticesPerEdge; ++x)
		{
			ASSERT(vertexIndex < numVertices);
			pushVertex(center + rotation * (radius * vec3(x * distanceBetweenVertices, 1.f, z * distanceBetweenVertices)), {}, {}, {}, {}, 0);

			if (z < numVerticesPerEdge - 1 && x < numVerticesPerEdge - 1)
			{
				ASSERT(triangleIndex < numTriangles);
				pushTriangle(z * numVerticesPerEdge + x, (z + 1) * numVerticesPerEdge + x, (z + 1) * numVerticesPerEdge + (x + 1));
				ASSERT(triangleIndex < numTriangles);
				pushTriangle((z + 1) * numVerticesPerEdge + (x + 1), z * numVerticesPerEdge + (x + 1), z * numVerticesPerEdge + x);

				triangleIndex += 2;
			}

			++vertexIndex;
		}
	}

	const uint32 ringSize = (numVerticesPerEdge - 1) * 4;

	// Sides
	uint32 y = 1;

	vec3 directions[] = { vec3(1.f, 0.f, 0.f), vec3(0.f, 0.f, 1.f), vec3(-1.f, 0.f, 0.f), vec3(0.f, 0.f, -1.f) };
	uint32 directionIndex = 0;

	vec3 currentPos(0.f, 1.f - y * distanceBetweenVertices, 0.f);

	uint32 ringStart = vertexIndex;

	while (y < numVerticesPerEdge)
	{
		vec3 direction = directions[directionIndex];
		for (uint32 i = 0; i < numVerticesPerEdge - 1; ++i)
		{
			ASSERT(vertexIndex < numVertices);
			pushVertex(desc.center + desc.rotation * (radius * currentPos), {}, {}, {}, {}, 0);
			currentPos += distanceBetweenVertices * direction;

			uint32 top, topLeft;
			if (y == 1)
			{
				switch (directionIndex)
				{
				case 0:
					top = i;
					topLeft = top + 1;
					break;
				case 1:
					top = i * numVerticesPerEdge + (numVerticesPerEdge - 1);
					topLeft = top + numVerticesPerEdge;
					break;
				case 2:
					top = numVerticesPerEdge * numVerticesPerEdge - 1 - i;
					topLeft = top - 1;
					break;
				case 3:
					top = (numVerticesPerEdge - 1 - i) * numVerticesPerEdge;
					topLeft = top - numVerticesPerEdge;
					break;
				}
			}
			else
			{
				top = vertexIndex - ringSize;
				topLeft = top + 1;
				if (ringStart == topLeft)
				{
					topLeft -= ringSize;
				}
			}

			uint32 left = vertexIndex + 1;
			if (ringStart + ringSize == left)
			{
				left -= ringSize;
			}

			ASSERT(triangleIndex < numTriangles);
			pushTriangle(topLeft, left, vertexIndex);
			ASSERT(triangleIndex < numTriangles);
			pushTriangle(vertexIndex, top, topLeft);

			triangleIndex += 2;

			++vertexIndex;
		}

		directionIndex = (directionIndex + 1) % 4;
		if (directionIndex == 0)
		{
			++y;
			ringStart = vertexIndex;
			currentPos.y = 1.f - y * distanceBetweenVertices;
		}
	}

	// Bottom face
	uint32 firstOfLastRing = vertexIndex - ringSize;
	for (uint32 z = 1; z < numVerticesPerEdge - 1; ++z)
	{
		for (uint32 x = 1; x < numVerticesPerEdge - 1; ++x)
		{
			ASSERT(vertexIndex < numVertices);
			pushVertex(desc.center + desc.rotation * (radius * vec3(x * distanceBetweenVertices, 0.f, z * distanceBetweenVertices)), {}, {}, {}, {}, 0);

			uint32 top = vertexIndex - (numVerticesPerEdge - 2);
			if (z == 1)
			{
				top = firstOfLastRing + x;
			}

			uint32 right = vertexIndex + 1;
			uint32 topRight = vertexIndex + 1 - (numVerticesPerEdge - 2);
			if (x == numVerticesPerEdge - 2)
			{
				right = firstOfLastRing + numVerticesPerEdge - 1 + z;
				topRight = right - 1;
			}
			else if (z == 1)
			{
				topRight = top + 1;
			}

			// Only valid if x == 1
			uint32 left = firstOfLastRing + ringSize - z;

			if (x == 1)
			{
				uint32 topLeft = left + 1;
				if (firstOfLastRing + ringSize == topLeft)
				{
					topLeft -= ringSize;
				}
				ASSERT(triangleIndex < numTriangles);
				pushTriangle(topLeft, top, vertexIndex);
				ASSERT(triangleIndex < numTriangles);
				pushTriangle(vertexIndex, left, topLeft);

				triangleIndex += 2;
			}

			ASSERT(triangleIndex < numTriangles);
			pushTriangle(top, topRight, right);
			ASSERT(triangleIndex < numTriangles);
			pushTriangle(right, vertexIndex, top);

			triangleIndex += 2;

			if (z == numVerticesPerEdge - 2)
			{
				uint32 bottom = firstOfLastRing + (numVerticesPerEdge - 1) * 3 - x;
				uint32 bottomLeft = bottom + 1;
				uint32 bottomRight = bottom - 1;

				if (x == 1)
				{
					ASSERT(triangleIndex < numTriangles);
					pushTriangle(left, vertexIndex, bottom);
					ASSERT(triangleIndex < numTriangles);
					pushTriangle(bottom, bottomLeft, left);

					triangleIndex += 2;
				}

				ASSERT(triangleIndex < numTriangles);
				pushTriangle(vertexIndex, right, bottomRight);
				ASSERT(triangleIndex < numTriangles);
				pushTriangle(bottomRight, bottom, vertexIndex);

				triangleIndex += 2;
			}

			++vertexIndex;
		}
	}

	if (desc.numIntervals == 1)
	{
		// Connect last two faces
		ASSERT(numVertices == 8);

		pushTriangle(4, 5, 7);
		pushTriangle(5, 6, 7);

		triangleIndex += 2;
	}

	ASSERT(vertexIndex == numVertices);
	ASSERT(triangleIndex == numTriangles);
}

void mesh_builder::pushSphere(const sphere_mesh_desc& desc, bool flipWindingOrder)
{
	uint32 slices = desc.slices;
	uint32 rows = desc.rows;
	vec3 center = desc.center;
	float radius = desc.radius;

	ASSERT(slices > 2);
	ASSERT(rows > 0);

	float vertDeltaAngle = M_PI / (rows + 1);
	float horzDeltaAngle = 2.f * M_PI / slices;

	auto [positionPtr, othersPtr, indexPtr, indexOffset] = beginPrimitive(slices * rows + 2, 2 * rows * slices);

	// Vertices
	pushVertex(center + vec3(0.f, -radius, 0.f), directionToPanoramaUV(vec3(0.f, -1.f, 0.f)), vec3(0.f, -1.f, 0.f), vec3(1.f, 0.f, 0.f), {}, 0);

	for (uint32 y = 0; y < rows; ++y)
	{
		float vertAngle = (y + 1) * vertDeltaAngle - M_PI;
		float vertexY = cos(vertAngle);
		float currentCircleRadius = sin(vertAngle);
		for (uint32 x = 0; x < slices; ++x)
		{
			float horzAngle = x * horzDeltaAngle;
			float vertexX = cos(horzAngle) * currentCircleRadius;
			float vertexZ = sin(horzAngle) * currentCircleRadius;
			vec3 pos(vertexX * radius, vertexY * radius, vertexZ * radius);
			vec3 nor(vertexX, vertexY, vertexZ);

			pushVertex(center + pos, directionToPanoramaUV(nor), normalize(nor), normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
		}
	}

	pushVertex(center + vec3(0.f, radius, 0.f), directionToPanoramaUV(vec3(0.f, 1.f, 0.f)), vec3(0.f, 1.f, 0.f), vec3(1.f, 0.f, 0.f), {}, 0);

	uint32 lastVertex = slices * rows + 2;

	// Indices
	for (uint32 x = 0; x < slices - 1; ++x)
	{
		pushTriangle(0, x + 1, x + 2);
	}
	pushTriangle(0, slices, 1);

	for (uint32 y = 0; y < rows - 1; ++y)
	{
		for (uint32 x = 0; x < slices - 1; ++x)
		{
			pushTriangle(y * slices + 1 + x, (y + 1) * slices + 2 + x, y * slices + 2 + x);
			pushTriangle(y * slices + 1 + x, (y + 1) * slices + 1 + x, (y + 1) * slices + 2 + x);
		}
		pushTriangle((uint32)(y * slices + slices), (uint32)((y + 1) * slices + 1), (uint32)(y * slices + 1));
		pushTriangle((uint32)(y * slices + slices), (uint32)((y + 1) * slices + slices), (uint32)((y + 1) * slices + 1));
	}
	for (uint32 x = 0; x < slices - 1; ++x)
	{
		pushTriangle(lastVertex - 2 - x, lastVertex - 3u - x, lastVertex - 1);
	}
	pushTriangle(lastVertex - 1 - slices, lastVertex - 2, lastVertex - 1);
}

void mesh_builder::pushIcoSphere(const icosphere_mesh_desc& desc, bool flipWindingOrder)
{
	vec3 center = desc.center;
	float radius = desc.radius;
	uint32 refinement = desc.refinement;

	struct vert
	{
		vec3 p;
		vec3 n;
		vec3 t;
	};

	std::vector<vert> vertices;
	std::vector<indexed_triangle32> triangles;

	const float t = (1.f + sqrt(5.f)) / 2.f;

#define push_ico_vertex(p) { vec3 nor = normalize(p); vec3 px = nor * radius; vec3 tan = normalize(cross(vec3(0.f, 1.f, 0.f), nor)); vertices.push_back({px, nor, tan}); }

	push_ico_vertex(vec3(-1.f, t, 0));
	push_ico_vertex(vec3(1.f, t, 0));
	push_ico_vertex(vec3(-1.f, -t, 0));
	push_ico_vertex(vec3(1.f, -t, 0));

	push_ico_vertex(vec3(0, -1.f, t));
	push_ico_vertex(vec3(0, 1.f, t));
	push_ico_vertex(vec3(0, -1.f, -t));
	push_ico_vertex(vec3(0, 1.f, -t));

	push_ico_vertex(vec3(t, 0, -1.f));
	push_ico_vertex(vec3(t, 0, 1.f));
	push_ico_vertex(vec3(-t, 0, -1.f));
	push_ico_vertex(vec3(-t, 0, 1.f));

	triangles.push_back({ 0, 11, 5 });
	triangles.push_back({ 0, 5, 1 });
	triangles.push_back({ 0, 1, 7 });
	triangles.push_back({ 0, 7, 10 });
	triangles.push_back({ 0, 10, 11 });
	triangles.push_back({ 1, 5, 9 });
	triangles.push_back({ 5, 11, 4 });
	triangles.push_back({ 11, 10, 2 });
	triangles.push_back({ 10, 7, 6 });
	triangles.push_back({ 7, 1, 8 });
	triangles.push_back({ 3, 9, 4 });
	triangles.push_back({ 3, 4, 2 });
	triangles.push_back({ 3, 2, 6 });
	triangles.push_back({ 3, 6, 8 });
	triangles.push_back({ 3, 8, 9 });
	triangles.push_back({ 4, 9, 5 });
	triangles.push_back({ 2, 4, 11 });
	triangles.push_back({ 6, 2, 10 });
	triangles.push_back({ 8, 6, 7 });
	triangles.push_back({ 9, 8, 1 });

	std::unordered_map<uint32, uint32> midpoints;

	auto getMiddlePoint = [&midpoints, &vertices, radius](uint32 a, uint32 b)
	{
		uint32 edge = (min(a, b) << 16) | (max(a, b));
		auto it = midpoints.find(edge);
		if (it == midpoints.end())
		{
			vec3 point1 = vertices[a].p;
			vec3 point2 = vertices[b].p;

			vec3 center = 0.5f * (point1 + point2);
			push_ico_vertex(center);

			uint32 index = (uint32)vertices.size() - 1;

			midpoints.insert({ edge, index });
			return index;
		}

		return it->second;
	};

	for (uint32 r = 0; r < refinement; ++r)
	{
		std::vector<indexed_triangle32> refinedTriangles;

		for (uint32 tri = 0; tri < (uint32)triangles.size(); ++tri)
		{
			indexed_triangle32& t = triangles[tri];

			uint32 a = getMiddlePoint(t.a, t.b);
			uint32 b = getMiddlePoint(t.b, t.c);
			uint32 c = getMiddlePoint(t.c, t.a);

			refinedTriangles.push_back({ t.a, a, c });
			refinedTriangles.push_back({ t.b, b, a });
			refinedTriangles.push_back({ t.c, c, b });
			refinedTriangles.push_back({ a, b, c });
		}

		triangles = refinedTriangles;
	}

	auto [positionPtr, othersPtr, indexPtr, indexOffset] = beginPrimitive((uint32)vertices.size(), (uint32)triangles.size());

	for (const vert& v : vertices)
	{
		pushVertex(v.p + center, {}, v.n, v.t, {}, 0);
	}

	for (auto t : triangles)
	{
		pushTriangle(t.a, t.b, t.c);
	}

#undef push_ico_vertex
}

void mesh_builder::pushCapsule(const capsule_mesh_desc& desc, bool flipWindingOrder)
{
	uint32 slices = desc.slices;
	uint32 rows = desc.rows;
	float height = desc.height;
	float radius = desc.radius;
	quat rotation = desc.rotation;
	vec3 center = desc.center;

	ASSERT(slices > 2);
	ASSERT(rows > 0);
	ASSERT(rows % 2 == 1);

	float vertDeltaAngle = M_PI / (rows + 1);
	float horzDeltaAngle = 2.f * M_PI / slices;
	float halfHeight = 0.5f * height;
	float texStretch = radius / (radius + halfHeight);

	auto [positionPtr, othersPtr, indexPtr, indexOffset] = beginPrimitive(slices * (rows + 1) + 2, 2 * (rows + 1) * slices);

	// Vertices
	pushVertex(rotation * vec3(0.f, -radius - halfHeight, 0.f) + center, 
		directionToPanoramaUV(vec3(0.f, -1.f, 0.f)), 
		rotation * vec3(0.f, -1.f, 0.f), 
		rotation * vec3(1.f, 0.f, 0.f), {}, 0);

	for (uint32 y = 0; y < rows / 2 + 1; ++y)
	{
		float vertAngle = (y + 1) * vertDeltaAngle - M_PI;
		float vertexY = cosf(vertAngle);
		float currentCircleRadius = sinf(vertAngle);
		for (uint32 x = 0; x < slices; ++x)
		{
			float horzAngle = x * horzDeltaAngle;
			float vertexX = cosf(horzAngle) * currentCircleRadius;
			float vertexZ = sinf(horzAngle) * currentCircleRadius;
			vec3 pos(vertexX * radius, vertexY * radius - halfHeight, vertexZ * radius);
			vec3 nor(vertexX, vertexY, vertexZ);

			vec2 uv = directionToPanoramaUV(nor);
			uv.y *= texStretch;
			pushVertex(rotation * pos + center, uv, rotation * normalize(nor), rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
		}
	}
	for (uint32 y = 0; y < rows / 2 + 1; ++y)
	{
		float vertAngle = (y + rows / 2 + 1) * vertDeltaAngle - M_PI;
		float vertexY = cosf(vertAngle);
		float currentCircleRadius = sinf(vertAngle);
		for (uint32 x = 0; x < slices; ++x)
		{
			float horzAngle = x * horzDeltaAngle;
			float vertexX = cosf(horzAngle) * currentCircleRadius;
			float vertexZ = sinf(horzAngle) * currentCircleRadius;
			vec3 pos(vertexX * radius, vertexY * radius + halfHeight, vertexZ * radius);
			vec3 nor(vertexX, vertexY, vertexZ);

			vec2 uv = directionToPanoramaUV(nor);
			uv.y *= texStretch;
			pushVertex(rotation * pos + center, uv, rotation * normalize(nor), rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
		}
	}
	pushVertex(rotation * vec3(0.f, radius + halfHeight, 0.f) + center, 
		directionToPanoramaUV(vec3(0.f, 1.f, 0.f)), 
		rotation * vec3(0.f, 1.f, 0.f), 
		rotation * vec3(1.f, 0.f, 0.f), {}, 0);

	uint32 lastVertex = slices * (rows + 1) + 2;

	// Indices
	for (uint32 x = 0; x < slices - 1; ++x)
	{
		pushTriangle(0, x + 1, x + 2);
	}
	pushTriangle(0, slices, 1);
	for (uint32 y = 0; y < rows; ++y)
	{
		for (uint32 x = 0; x < slices - 1; ++x)
		{
			pushTriangle(y * slices + 1 + x, (y + 1) * slices + 2 + x, y * slices + 2 + x);
			pushTriangle(y * slices + 1 + x, (y + 1) * slices + 1 + x, (y + 1) * slices + 2 + x);
		}
		pushTriangle(y * slices + slices, (y + 1) * slices + 1, y * slices + 1);
		pushTriangle(y * slices + slices, (y + 1) * slices + slices, (y + 1) * slices + 1);
	}
	for (uint32 x = 0; x < slices - 1; ++x)
	{
		pushTriangle(lastVertex - 2 - x, lastVertex - 3 - x, lastVertex - 1);
	}
	pushTriangle(lastVertex - 1 - slices, lastVertex - 2, lastVertex - 1);
}

struct sincos
{
	float sin, cos;
};

void mesh_builder::pushCylinder(const cylinder_mesh_desc& desc, bool flipWindingOrder)
{
	uint32 slices = desc.slices;
	float height = desc.height;
	float radius = desc.radius;
	quat rotation = desc.rotation;
	vec3 center = desc.center;

	ASSERT(slices > 2);

	float horzDeltaAngle = 2.f * M_PI / slices;
	float halfHeight = height * 0.5f;
	
	auto [positionPtr, othersPtr, indexPtr, indexOffset] = beginPrimitive(4 * slices + 2, 4 * slices);

	sincos* angles = (sincos*)alloca(sizeof(sincos) * slices);
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		angles[x] = { sinf(horzAngle), cosf(horzAngle) };
	}

	pushVertex(center + rotation * vec3(0.f, -halfHeight, 0.f), vec2(0.25f, 0.75f), rotation * vec3(0.f, -1.f, 0.f), rotation * vec3(1.f, 0.f, 0.f), {}, 0);

	// Bottom row, normal down
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = angles[x].cos;
		float vertexZ = angles[x].sin;
		vec3 pos(vertexX * radius, -halfHeight, vertexZ * radius);
		vec3 nor(0.f, -1.f, 0.f);

		pushVertex(center + rotation * pos, remap(vec2(vertexX, vertexZ), -1.f, 1.f, vec2(0.f, 0.5f), vec2(0.5f, 1.f)), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	// Bottom row, normal around
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = angles[x].cos;
		float vertexZ = angles[x].sin;
		vec3 pos(vertexX * radius, -halfHeight, vertexZ * radius);
		vec3 nor(vertexX, 0.f, vertexZ);

		pushVertex(center + rotation * pos, vec2(x / (float)(slices - 1), 0.5f), rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
	}

	// Top row, normal around
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = angles[x].cos;
		float vertexZ = angles[x].sin;
		vec3 pos(vertexX * radius, halfHeight, vertexZ * radius);
		vec3 nor(vertexX, 0.f, vertexZ);

		pushVertex(center + rotation * pos, vec2(x / (float)(slices - 1), 0.f), rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
	}

	// Top row, normal up
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = angles[x].cos;
		float vertexZ = angles[x].sin;
		vec3 pos(vertexX * radius, halfHeight, vertexZ * radius);
		vec3 nor(0.f, 1.f, 0.f);

		pushVertex(center + rotation * pos, remap(vec2(vertexX, vertexZ), -1.f, 1.f, vec2(0.5f, 0.5f), vec2(1.f, 1.f)), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	pushVertex(center + rotation * vec3(0.f, halfHeight, 0.f), vec2(0.75f, 0.75f), rotation * vec3(0.f, 1.f, 0.f), rotation * vec3(1.f, 0.f, 0.f), {}, 0);

	uint32 lastVertex = 4 * slices + 2;

	// Indices
	for (uint32 x = 0; x < slices - 1u; ++x)
	{
		pushTriangle(0, x + 1, x + 2);
	}
	pushTriangle(0, slices, 1);

	for (uint32 x = 0; x < slices - 1u; ++x)
	{
		pushTriangle(slices + 1 + x, 2 * slices + 2 + x, slices + 2 + x);
		pushTriangle(slices + 1 + x, 2 * slices + 1 + x, 2 * slices + 2 + x);
	}
	pushTriangle(slices + slices, 2 * slices + 1, slices + 1);
	pushTriangle(slices + slices, 2 * slices + slices, 2 * slices + 1);

	for (uint32 x = 0; x < slices - 1u; ++x)
	{
		pushTriangle(lastVertex - 2 - x, lastVertex - 3 - x, lastVertex - 1);
	}
	pushTriangle(lastVertex - 1 - slices, lastVertex - 2, lastVertex - 1);
}

void mesh_builder::pushHollowCylinder(const hollow_cylinder_mesh_desc& desc, bool flipWindingOrder)
{
	uint32 slices = desc.slices;
	float height = desc.height;
	float radius = desc.radius;
	float innerRadius = desc.innerRadius;
	quat rotation = desc.rotation;
	vec3 center = desc.center;

	ASSERT(slices > 2);

	float horzDeltaAngle = 2.f * M_PI / slices;
	float halfHeight = height * 0.5f;

	auto [positionPtr, othersPtr, indexPtr, indexOffset] = beginPrimitive(8 * slices, 8 * slices);

	sincos* angles = (sincos*)alloca(sizeof(sincos) * slices);
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		angles[x] = { sinf(horzAngle), cosf(horzAngle) };
	}

	float radiusRelation = innerRadius / radius;

	// Bottom outer row, normal down
	for (uint32 x = 0; x < slices; ++x)
	{
		float vertexX = angles[x].cos;
		float vertexZ = angles[x].sin;
		vec3 pos(vertexX * radius, -halfHeight, vertexZ * radius);
		vec3 nor(0.f, -1.f, 0.f);

		pushVertex(center + rotation * pos, remap(vec2(vertexX, vertexZ), -1.f, 1.f, vec2(0.f, 0.5f), vec2(0.5f, 1.f)), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	// Bottom row, normal around
	for (uint32 x = 0; x < slices; ++x)
	{
		float vertexX = angles[x].cos;
		float vertexZ = angles[x].sin;
		vec3 pos(vertexX * radius, -halfHeight, vertexZ * radius);
		vec3 nor(vertexX, 0.f, vertexZ);

		pushVertex(center + rotation * pos, vec2(x / (float)(slices - 1), 0.5f), rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
	}

	// Top row, normal around
	for (uint32 x = 0; x < slices; ++x)
	{
		float vertexX = angles[x].cos;
		float vertexZ = angles[x].sin;
		vec3 pos(vertexX * radius, halfHeight, vertexZ * radius);
		vec3 nor(vertexX, 0.f, vertexZ);

		pushVertex(center + rotation * pos, vec2(x / (float)(slices - 1), 0.f), rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
	}

	// Top row, normal up
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * radius, halfHeight, vertexZ * radius);
		vec3 nor(0.f, 1.f, 0.f);

		pushVertex(center + rotation * pos, remap(vec2(vertexX, vertexZ), -1.f, 1.f, vec2(0.5f, 0.5f), vec2(1.f, 1.f)), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	vec2 innerUVMin = vec2(0.25f, 0.75f) - 0.25f * radiusRelation;
	vec2 innerUVMax = vec2(0.25f, 0.75f) + 0.25f * radiusRelation;

	// Bottom inner row, normal down
	for (uint32 x = 0; x < slices; ++x)
	{
		float vertexX = angles[x].cos;
		float vertexZ = angles[x].sin;
		vec3 pos(vertexX * innerRadius, -halfHeight, vertexZ * innerRadius);
		vec3 nor(0.f, -1.f, 0.f);

		pushVertex(center + rotation * pos, remap(vec2(vertexX, vertexZ), -1.f, 1.f, innerUVMin, innerUVMax), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	// Bottom inner row, normal inside
	for (uint32 x = 0; x < slices; ++x)
	{
		float vertexX = angles[x].cos;
		float vertexZ = angles[x].sin;
		vec3 pos(vertexX * innerRadius, -halfHeight, vertexZ * innerRadius);
		vec3 nor(-vertexX, 0.f, -vertexZ);

		pushVertex(center + rotation * pos, vec2(x / (float)(slices - 1), 0.5f), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	// Top inner row, normal inside
	for (uint32 x = 0; x < slices; ++x)
	{
		float vertexX = angles[x].cos;
		float vertexZ = angles[x].sin;
		vec3 pos(vertexX * innerRadius, halfHeight, vertexZ * innerRadius);
		vec3 nor(-vertexX, 0.f, -vertexZ);

		pushVertex(center + rotation * pos, vec2(x / (float)(slices - 1), 0.f), rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
	}

	// Top inner row, normal up
	for (uint32 x = 0; x < slices; ++x)
	{
		float vertexX = angles[x].cos;
		float vertexZ = angles[x].sin;
		vec3 pos(vertexX * innerRadius, halfHeight, vertexZ * innerRadius);
		vec3 nor(0.f, 1.f, 0.f);

		pushVertex(center + rotation * pos, remap(vec2(vertexX, vertexZ), -1.f, 1.f, innerUVMin + vec2(0.5f, 0.f), innerUVMax + vec2(0.5f, 0.f)), rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	// Indices
	uint32 firstInnerBottomVertex = 4 * slices;

	for (uint32 x = 0; x < slices - 1u; ++x)
	{
		pushTriangle(firstInnerBottomVertex + x, x, x + 1);
		pushTriangle(firstInnerBottomVertex + x, x + 1, firstInnerBottomVertex + x + 1);
	}
	pushTriangle(firstInnerBottomVertex + slices - 1u, slices - 1u, 0);
	pushTriangle(firstInnerBottomVertex + slices - 1u, 0, firstInnerBottomVertex);

	for (uint32 x = 0; x < slices - 1u; ++x)
	{
		pushTriangle(slices + x, 2 * slices + 1 + x, slices + 1 + x);
		pushTriangle(slices + x, 2 * slices + x, 2 * slices + 1 + x);
	}
	pushTriangle(slices + slices - 1, 2 * slices, slices);
	pushTriangle(slices + slices - 1, 2 * slices + slices - 1, 2 * slices);

	uint32 firstInnerTopVertex = 7 * slices;
	uint32 firstOuterTopVertex = 3 * slices;

	for (uint32 x = 0; x < slices - 1u; ++x)
	{
		pushTriangle(firstInnerTopVertex + x, firstOuterTopVertex + x + 1, firstOuterTopVertex + x);
		pushTriangle(firstInnerTopVertex + x, firstInnerTopVertex + x + 1, firstOuterTopVertex + x + 1);
	}
	pushTriangle(firstInnerTopVertex + slices - 1u, firstOuterTopVertex + 0, firstOuterTopVertex + slices - 1u);
	pushTriangle(firstInnerTopVertex + slices - 1u, firstInnerTopVertex, firstOuterTopVertex + 0);

	firstInnerBottomVertex = 5 * slices;
	firstInnerTopVertex = 6 * slices;

	for (uint32 x = 0; x < slices - 1u; ++x)
	{
		pushTriangle(firstInnerBottomVertex + x, firstInnerBottomVertex + 1 + x, firstInnerTopVertex + 1 + x);
		pushTriangle(firstInnerBottomVertex + x, firstInnerTopVertex + 1 + x, firstInnerTopVertex + x);
	}
	pushTriangle(firstInnerBottomVertex + slices - 1, firstInnerBottomVertex, firstInnerTopVertex);
	pushTriangle(firstInnerBottomVertex + slices - 1, firstInnerTopVertex, firstInnerTopVertex + slices - 1);
}

void mesh_builder::pushArrow(const arrow_mesh_desc& desc, bool flipWindingOrder)
{
	float shaftLength = desc.shaftLength;
	float shaftRadius = desc.shaftRadius;
	float headLength = desc.headLength;
	float headRadius = desc.headRadius;
	quat rotation = desc.rotation;
	vec3 base = desc.base;
	uint32 slices = desc.slices;

	ASSERT(slices > 2);

	float horzDeltaAngle = 2.f * M_PI / slices;

	auto [positionPtr, othersPtr, indexPtr, indexOffset] = beginPrimitive(7 * slices + 1, 7 * slices);

	vec2 uv(0.f, 0.f);
	pushVertex(base, uv, rotation * vec3(0.f, -1.f, 0.f), rotation * vec3(1.f, 0.f, 0.f), {}, 0);

	// Bottom row, normal down
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * shaftRadius, 0.f, vertexZ * shaftRadius);
		vec3 nor(0.f, -1.f, 0.f);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	// Bottom row, normal around
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * shaftRadius, 0.f, vertexZ * shaftRadius);
		vec3 nor(vertexX, 0.f, vertexZ);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
	}

	// Top row, normal around
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * shaftRadius, shaftLength, vertexZ * shaftRadius);
		vec3 nor(vertexX, 0.f, vertexZ);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
	}

	// Top row, normal down
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * shaftRadius, shaftLength, vertexZ * shaftRadius);
		vec3 nor(0.f, -1.f, 0.f);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	// Top outer row, normal down
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * headRadius, shaftLength, vertexZ * headRadius);
		vec3 nor(0.f, -1.f, 0.f);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	vec2 normal2D = normalize(vec2(headLength, headRadius));

	// Top outer row, normal around
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * headRadius, shaftLength, vertexZ * headRadius);
		vec3 nor(vertexX * normal2D.x, normal2D.y, vertexZ * normal2D.x);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	// Top vertex
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(0.f, shaftLength + headLength, 0.f);
		vec3 nor(vertexX * normal2D.x, normal2D.y, vertexZ * normal2D.x);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	// Indices
	for (uint32 x = 0; x < slices - 1; ++x)
	{
		pushTriangle(0, x + 1, x + 2);
	}
	pushTriangle(0, slices, 1);

	for (uint32 y = 1; y < 7; y += 2)
	{
		for (uint32 x = 0; x < slices - 1; ++x)
		{
			pushTriangle(y * slices + 1 + x, (y + 1) * slices + 2 + x, y * slices + 2 + x);
			pushTriangle(y * slices + 1 + x, (y + 1) * slices + 1 + x, (y + 1) * slices + 2 + x);
		}
		pushTriangle(y * slices + slices, (y + 1) * slices + 1, y * slices + 1);
		pushTriangle(y * slices + slices, (y + 1) * slices + slices, (y + 1) * slices + 1);
	}
}

void mesh_builder::pushTorus(const torus_mesh_desc& desc, bool flipWindingOrder)
{
	float torusRadius = desc.torusRadius;
	float tubeRadius = desc.tubeRadius;
	uint32 slices = desc.slices;
	uint32 segments = desc.segments;
	vec3 center = desc.center;
	quat rotation = desc.rotation * quat(vec3(1.f, 0.f, 0.f), deg2rad(90.f));

	ASSERT(slices > 2);
	ASSERT(segments > 2);

	float tubeDeltaAngle = 2.f * M_PI / slices;
	float torusDeltaAngle = 2.f * M_PI / segments;

	auto [positionPtr, othersPtr, indexPtr, indexOffset] = beginPrimitive(segments * slices, segments * slices * 2);
	
	vec2 uv(0.f, 0.f);

	for (uint32 s = 0; s < segments; ++s)
	{
		float segmentAngle = s * torusDeltaAngle;
		quat segmentRotation(vec3(0.f, 0.f, 1.f), segmentAngle);

		vec3 segmentOffset = segmentRotation * vec3(torusRadius, 0.f, 0.f);

		for (uint32 x = 0; x < slices; ++x)
		{
			float horzAngle = x * tubeDeltaAngle;
			float vertexX = cosf(horzAngle);
			float vertexZ = sinf(horzAngle);
			vec3 pos = segmentRotation * vec3(vertexX * tubeRadius, 0.f, vertexZ * tubeRadius) + segmentOffset;
			vec3 nor = segmentRotation * vec3(vertexX, 0.f, vertexZ);

			pushVertex(center + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
		}
	}

	for (uint32 y = 0; y < segments - 1u; ++y)
	{
		for (uint32 x = 0; x < slices - 1u; ++x)
		{
			pushTriangle(y * slices + x, (y + 1) * slices + 1 + x, y * slices + 1 + x);
			pushTriangle(y * slices + x, (y + 1) * slices + x, (y + 1) * slices + 1 + x);
		}
		pushTriangle(y * slices + slices - 1, (y + 1) * slices, y * slices);
		pushTriangle(y * slices + slices - 1, (y + 1) * slices + slices - 1, (y + 1) * slices);
	}

	uint32 y = segments - 1u;
	for (uint32 x = 0; x < slices - 1u; ++x)
	{
		pushTriangle(y * slices + x, 1 + x, y * slices + 1 + x);
		pushTriangle(y * slices + x, x, 1 + x);
	}
	pushTriangle(y * slices + slices - 1, 0, y * slices);
	pushTriangle(y * slices + slices - 1, slices - 1, 0);
}

void mesh_builder::pushMace(const mace_mesh_desc& desc, bool flipWindingOrder)
{
	float shaftLength = desc.shaftLength;
	float shaftRadius = desc.shaftRadius;
	float headLength = desc.headLength;
	float headRadius = desc.headRadius;
	quat rotation = desc.rotation;
	vec3 base = desc.base;
	uint32 slices = desc.slices;

	ASSERT(slices > 2);

	float horzDeltaAngle = 2.f * M_PI / slices;
	
	auto [positionPtr, othersPtr, indexPtr, indexOffset] = beginPrimitive(8 * slices + 2, 8 * slices);

	vec2 uv(0.f, 0.f);
	pushVertex(base, uv, rotation * vec3(0.f, -1.f, 0.f), rotation * vec3(1.f, 0.f, 0.f), {}, 0);

	// Bottom row, normal down
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * shaftRadius, 0.f, vertexZ * shaftRadius);
		vec3 nor(0.f, -1.f, 0.f);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	// Bottom row, normal around
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * shaftRadius, 0.f, vertexZ * shaftRadius);
		vec3 nor(vertexX, 0.f, vertexZ);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
	}

	// Top row, normal around
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * shaftRadius, shaftLength, vertexZ * shaftRadius);
		vec3 nor(vertexX, 0.f, vertexZ);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
	}

	// Top row, normal down
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * shaftRadius, shaftLength, vertexZ * shaftRadius);
		vec3 nor(0.f, -1.f, 0.f);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	// Top outer row, normal down
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * headRadius, shaftLength, vertexZ * headRadius);
		vec3 nor(0.f, -1.f, 0.f);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
	}

	// Top outer row, normal around
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * headRadius, shaftLength, vertexZ * headRadius);
		vec3 nor(vertexX, 0.f, vertexZ);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
	}

	// Top top outer row, normal around
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * headRadius, shaftLength + headLength, vertexZ * headRadius);
		vec3 nor(vertexX, 0.f, vertexZ);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * normalize(cross(vec3(0.f, 1.f, 0.f), nor)), {}, 0);
	}

	// Top top outer row, normal up
	for (uint32 x = 0; x < slices; ++x)
	{
		float horzAngle = x * horzDeltaAngle;
		float vertexX = cosf(horzAngle);
		float vertexZ = sinf(horzAngle);
		vec3 pos(vertexX * headRadius, shaftLength + headLength, vertexZ * headRadius);
		vec3 nor(0.f, 1.f, 0.f);

		pushVertex(base + rotation * pos, uv, rotation * nor, rotation * vec3(1.f, 0.f, 0.f), {}, 0);
	}

	pushVertex(base + rotation * vec3(0.f, shaftLength + headLength, 0.f), uv, rotation * vec3(0.f, 1.f, 0.f), rotation * vec3(1.f, 0.f, 0.f), {}, 0);

	uint32 lastVertex = 8 * slices + 2;

	// Indices
	for (uint32 x = 0; x < slices - 1u; ++x)
	{
		pushTriangle(0, x + 1, x + 2);
	}
	pushTriangle(0, slices, 1);

	for (uint32 y = 1; y < 7; y += 2)
	{
		for (uint32 x = 0; x < slices - 1u; ++x)
		{
			pushTriangle(y * slices + 1 + x, (y + 1) * slices + 2 + x, y * slices + 2 + x);
			pushTriangle(y * slices + 1 + x, (y + 1) * slices + 1 + x, (y + 1) * slices + 2 + x);
		}
		pushTriangle(y * slices + slices, (y + 1) * slices + 1, y * slices + 1);
		pushTriangle(y * slices + slices, (y + 1) * slices + slices, (y + 1) * slices + 1);
	}

	for (uint32 x = 0; x < slices - 1u; ++x)
	{
		pushTriangle(lastVertex - 2 - x, lastVertex - 3 - x, lastVertex - 1);
	}
	pushTriangle(lastVertex - 1 - slices, lastVertex - 2, lastVertex - 1);
}

void mesh_builder::pushMesh(const submesh_asset& mesh, float scale, bounding_box* aabb)
{
	uint32 numVertices = (uint32)mesh.positions.size();
	uint32 numFaces = (uint32)mesh.triangles.size();

	if (indexType == mesh_index_uint16)
	{
		ASSERT(numVertices - 1 <= UINT16_MAX);
	}

	auto [positionPtr, othersPtr, indexPtr, indexOffset] = beginPrimitive(numVertices, numFaces);
	auto vertexOthers = othersPtr;

	vec3 position(0.f, 0.f, 0.f);
	vec3 normal(0.f, 0.f, 0.f);
	vec3 tangent(0.f, 0.f, 0.f);
	vec2 uv(0.f, 0.f);
	uint32 vertexColor = 0;
	skinning_weights skin = {};

	bool hasPositions = true;
	bool hasUVs = !mesh.uvs.empty();
	bool hasNormals = !mesh.normals.empty();
	bool hasTangents = !mesh.tangents.empty();
	bool hasVertexColors = !mesh.colors.empty();;
	bool hasSkin = !mesh.skin.empty();

	if (aabb)
	{
		*aabb = bounding_box::negativeInfinity();
	}

	for (uint32 i = 0; i < numVertices; ++i)
	{
		position = mesh.positions[i] * scale;
		if (aabb)
		{
			aabb->grow(position);
		}

		if (hasUVs)
		{
			uv = mesh.uvs[i];
		}
		if (hasNormals)
		{
			normal = mesh.normals[i];
		}
		if (hasTangents)
		{
			tangent = mesh.tangents[i];
		}
		if (hasVertexColors)
		{
			vertexColor = mesh.colors[i];
		}
		if (hasSkin)
		{
			skin = mesh.skin[i];
		}

		pushVertex(position, uv, normal, tangent, skin, vertexColor);
	}

	const bool flipWindingOrder = false;
	for (uint32 i = 0; i < numFaces; ++i)
	{
		const indexed_triangle16& tri = mesh.triangles[i];
		pushTriangle(tri.a, tri.b, tri.c);
	}
}

NODISCARD submesh_info mesh_builder::endSubmesh()
{
	uint32 firstVertex = totalNumVertices;
	uint32 firstTriangle = totalNumTriangles;

	submesh_info result;
	result.firstIndex = firstTriangle * 3;
	result.numIndices = numTrianglesInCurrentSubmesh * 3;
	result.baseVertex = firstVertex;
	result.numVertices = numVerticesInCurrentSubmesh;


	totalNumVertices += numVerticesInCurrentSubmesh;
	totalNumTriangles += numTrianglesInCurrentSubmesh;

	// Align the next index to a 16-byte boundary
	uint32 alignedNumTriangles = alignTo(totalNumTriangles, 8); // 8 triangles are 48 bytes, which is divisible by 16.
	uint32 missing = alignedNumTriangles - totalNumTriangles;
	if (missing > 0)
	{
		indexArena.allocate(indexSize * 3 * missing); // Push arena forward
	}
	totalNumTriangles = alignedNumTriangles;

	// Reset next submesh
	numVerticesInCurrentSubmesh = 0;
	numTrianglesInCurrentSubmesh = 0;

	++numSubmeshes;

	return result;
}

NODISCARD dx_mesh mesh_builder::createDXMesh()
{
	if (numVerticesInCurrentSubmesh > 0)
	{
		endSubmesh();
	}

	dx_mesh result;
	result.vertexBuffer.positions = createVertexBuffer(sizeof(vec3), totalNumVertices, positionArena.base());
	if (vertexFlags != mesh_creation_flags_with_positions)
	{
		result.vertexBuffer.others = createVertexBuffer(othersSize, totalNumVertices, othersArena.base(), true);
	}
	result.indexBuffer = createIndexBuffer(indexSize, totalNumTriangles * 3, indexArena.base());
	return result;
}

std::tuple<vec3*, uint8*, uint8*, uint32> mesh_builder::beginPrimitive(uint32 numVertices, uint32 numTriangles)
{
	vec3* positionPtr = (vec3*)positionArena.allocate(sizeof(vec3) * numVertices);
	uint8* othersPtr = (uint8*)othersArena.allocate(othersSize * numVertices);
	uint8* indexPtr = (uint8*)indexArena.allocate(indexSize * 3 * numTriangles);

	uint32 indexOffset = numVerticesInCurrentSubmesh;

	numVerticesInCurrentSubmesh += numVertices;
	numTrianglesInCurrentSubmesh += numTriangles;

	uint32 maxNumVertices = (indexType == mesh_index_uint16) ? UINT16_MAX : UINT32_MAX;
	ASSERT(numVerticesInCurrentSubmesh - 1 <= maxNumVertices);

	return { positionPtr, othersPtr, indexPtr, indexOffset };
}