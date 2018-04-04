//
// Copyright (C) Supernova Studio
// 
// File: isimpFty.cpp
//
// MEL Command: isimp
//
// Author: George Zhu
//

#include "isimpFty.h"

isimpFty::isimpFty()
//
//	Description:
//		meshOpFty constructor
//
{
	fComponentIDs.clear();
}

isimpFty::~isimpFty()
//
//	Description:
//		meshOpFty destructor
//
{}

void isimpFty::setMesh(MObject& mesh)
//
//	Description:
//		Sets the mesh object for the factory to operate on
//
{
	fMesh = mesh;
}

void isimpFty::setComponentList(MObject& componentList)
//
//	Description:
//		Sets the list of the components for the factory to operate on
//
{
	fComponentList = componentList;
}


void isimpFty::setComponentIDs(MIntArray& componentIDs)
//
//	Description:
//		Sets the list of the components for the factory to operate on
//
{
	fComponentIDs = componentIDs;
}

void isimpFty::setMeshOperation(MeshOperation operationType)
//
//	Description:
//		Sets the mesh operation for the factory to execute
//
{
	fOperationType = operationType;
}

void isimpFty::setVSAParams(int numProxies, int numIterations, double edgeSplitThres)
{
	//	Description:
	//		Sets VSA specific parameters
	//
	fNumIterations = numIterations;
	fNumProxies = numProxies;
	edgeSplitThreshold = edgeSplitThres;
}

MFn::Type isimpFty::getExpectedComponentType(MeshOperation operationType)
{
	switch (operationType) 
	{
	case kFlood: case kRefresh: case kGenerate: return MFn::kMeshPolygonComponent;
	case kDeleteProxyBySeed: return MFn::kMeshPolygonComponent;
		// FIXME paint should accept a list of connected polygons
	case kPaintProxyByFace: return MFn::kMeshPolygonComponent;
	case kAddProxyBySeed: return MFn::kMeshPolygonComponent;

	default: return MFn::kInvalid;
	}
}
