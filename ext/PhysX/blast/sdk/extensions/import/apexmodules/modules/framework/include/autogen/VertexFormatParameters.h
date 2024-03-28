// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.

// This file was generated by NvParameterized/scripts/GenParameterized.pl


#ifndef HEADER_VertexFormatParameters_h
#define HEADER_VertexFormatParameters_h

#include "NvParametersTypes.h"

#ifndef NV_PARAMETERIZED_ONLY_LAYOUTS
#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParameterizedTraits.h"
#include "NvParameters.h"
#include "NvTraitsInternal.h"
#endif

namespace nvidia
{
namespace apex
{

#if PX_VC
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())
#endif

namespace VertexFormatParametersNS
{

struct BufferFormat_Type;

struct BufferFormat_DynamicArray1D_Type
{
	BufferFormat_Type* buf;
	bool isAllocated;
	int32_t elementSize;
	int32_t arraySizes[1];
};

struct BufferFormat_Type
{
	NvParameterized::DummyStringStruct name;
	int32_t semantic;
	uint32_t id;
	uint32_t format;
	uint32_t access;
	bool serialize;
};

struct ParametersStruct
{

	uint32_t winding;
	bool hasSeparateBoneBuffer;
	BufferFormat_DynamicArray1D_Type bufferFormats;

};

static const uint32_t checksum[] = { 0xa7c1ed95, 0x570ed2b1, 0x55717659, 0x9951d139, };

} // namespace VertexFormatParametersNS

#ifndef NV_PARAMETERIZED_ONLY_LAYOUTS
class VertexFormatParameters : public NvParameterized::NvParameters, public VertexFormatParametersNS::ParametersStruct
{
public:
	VertexFormatParameters(NvParameterized::Traits* traits, void* buf = 0, int32_t* refCount = 0);

	virtual ~VertexFormatParameters();

	virtual void destroy();

	static const char* staticClassName(void)
	{
		return("VertexFormatParameters");
	}

	const char* className(void) const
	{
		return(staticClassName());
	}

	static const uint32_t ClassVersion = ((uint32_t)0 << 16) + (uint32_t)0;

	static uint32_t staticVersion(void)
	{
		return ClassVersion;
	}

	uint32_t version(void) const
	{
		return(staticVersion());
	}

	static const uint32_t ClassAlignment = 8;

	static const uint32_t* staticChecksum(uint32_t& bits)
	{
		bits = 8 * sizeof(VertexFormatParametersNS::checksum);
		return VertexFormatParametersNS::checksum;
	}

	static void freeParameterDefinitionTable(NvParameterized::Traits* traits);

	const uint32_t* checksum(uint32_t& bits) const
	{
		return staticChecksum(bits);
	}

	const VertexFormatParametersNS::ParametersStruct& parameters(void) const
	{
		VertexFormatParameters* tmpThis = const_cast<VertexFormatParameters*>(this);
		return *(static_cast<VertexFormatParametersNS::ParametersStruct*>(tmpThis));
	}

	VertexFormatParametersNS::ParametersStruct& parameters(void)
	{
		return *(static_cast<VertexFormatParametersNS::ParametersStruct*>(this));
	}

	virtual NvParameterized::ErrorType getParameterHandle(const char* long_name, NvParameterized::Handle& handle) const;
	virtual NvParameterized::ErrorType getParameterHandle(const char* long_name, NvParameterized::Handle& handle);

	void initDefaults(void);

protected:

	virtual const NvParameterized::DefinitionImpl* getParameterDefinitionTree(void);
	virtual const NvParameterized::DefinitionImpl* getParameterDefinitionTree(void) const;


	virtual void getVarPtr(const NvParameterized::Handle& handle, void*& ptr, size_t& offset) const;

private:

	void buildTree(void);
	void initDynamicArrays(void);
	void initStrings(void);
	void initReferences(void);
	void freeDynamicArrays(void);
	void freeStrings(void);
	void freeReferences(void);

	static bool mBuiltFlag;
	static NvParameterized::MutexType mBuiltFlagMutex;
};

class VertexFormatParametersFactory : public NvParameterized::Factory
{
	static const char* const vptr;

public:

	virtual void freeParameterDefinitionTable(NvParameterized::Traits* traits)
	{
		VertexFormatParameters::freeParameterDefinitionTable(traits);
	}

	virtual NvParameterized::Interface* create(NvParameterized::Traits* paramTraits)
	{
		// placement new on this class using mParameterizedTraits

		void* newPtr = paramTraits->alloc(sizeof(VertexFormatParameters), VertexFormatParameters::ClassAlignment);
		if (!NvParameterized::IsAligned(newPtr, VertexFormatParameters::ClassAlignment))
		{
			NV_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class VertexFormatParameters");
			paramTraits->free(newPtr);
			return 0;
		}

		memset(newPtr, 0, sizeof(VertexFormatParameters)); // always initialize memory allocated to zero for default values
		return NV_PARAM_PLACEMENT_NEW(newPtr, VertexFormatParameters)(paramTraits);
	}

	virtual NvParameterized::Interface* finish(NvParameterized::Traits* paramTraits, void* bufObj, void* bufStart, int32_t* refCount)
	{
		if (!NvParameterized::IsAligned(bufObj, VertexFormatParameters::ClassAlignment)
		        || !NvParameterized::IsAligned(bufStart, VertexFormatParameters::ClassAlignment))
		{
			NV_PARAM_TRAITS_WARNING(paramTraits, "Unaligned memory allocation for class VertexFormatParameters");
			return 0;
		}

		// Init NvParameters-part
		// We used to call empty constructor of VertexFormatParameters here
		// but it may call default constructors of members and spoil the data
		NV_PARAM_PLACEMENT_NEW(bufObj, NvParameterized::NvParameters)(paramTraits, bufStart, refCount);

		// Init vtable (everything else is already initialized)
		*(const char**)bufObj = vptr;

		return (VertexFormatParameters*)bufObj;
	}

	virtual const char* getClassName()
	{
		return (VertexFormatParameters::staticClassName());
	}

	virtual uint32_t getVersion()
	{
		return (VertexFormatParameters::staticVersion());
	}

	virtual uint32_t getAlignment()
	{
		return (VertexFormatParameters::ClassAlignment);
	}

	virtual const uint32_t* getChecksum(uint32_t& bits)
	{
		return (VertexFormatParameters::staticChecksum(bits));
	}
};
#endif // NV_PARAMETERIZED_ONLY_LAYOUTS

} // namespace apex
} // namespace nvidia

#if PX_VC
#pragma warning(pop)
#endif

#endif
