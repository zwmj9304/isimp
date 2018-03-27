//
// Copyright (C) Supernova Studio
// 
// File: isimpFtyAction.cpp
//
// MEL Command: isimp
//
// Author: George Zhu
//

#include "isimpFty.h"

// General Includes
//
#include <maya/MGlobal.h>
#include <maya/MIOStream.h>
#include <maya/MFloatPointArray.h>

// Function Sets
//
#include <maya/MFnMesh.h>

// Iterators
//
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshEdge.h>

#define CHECK_STATUS(st) if ((st) != MS::kSuccess) { break; }

MStatus isimpFty::doIt()
//
//	Description:
//		Performs the operation on the selected mesh and components
//
{
	MStatus status;

	// Get access to the mesh's function set
	//
	MFnMesh meshFn(fMesh);

	// Execute the requested operation
	//
	switch (fOperationType)
	{
	case kFlood: {
		status = doFlooding();
		CHECK_STATUS(status);
		break; }
	//case kGenerate: {
	//	status = doMeshing();
	//	CHECK_STATUS(status);
	//	break; }
	//case kRefresh: {
	//	status = doReFlooding();
	//	CHECK_STATUS(status);
	//	break; }
	//case kAddProxyBySeed: {
	//	status = doAddProxy();
	//	CHECK_STATUS(status);
	//	break; }
	//case kDeleteProxyBySeed: {
	//	status = doDelProxy();
	//	CHECK_STATUS(status);
	//	break; }
	//case kPaintProxyByFace: {
	//	status = doPaintProxy();
	//	CHECK_STATUS(status);
	//	break; }
	default:
		status = MS::kFailure;
		break;
	}

	return status;
}

MStatus isimpFty::doFlooding() {
	MStatus status;

	// Prepare mesh for Distortion Minimizing Flooding
	//
	MFnMesh meshFn(fMesh);

	// TODO clear proxy labels data stored in mesh

	// Start Variational Shape Approximation Routine
	//

	// Build FaceNeighbors data structure, maybe cache it?
	//
	status = buildFaceNeighbors();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Run Distortion Minimizing Flooding on given mesh using attributes
	// such as numProxies, numIterations, etc.
	//
	VSA::init(faceList, proxyList, fNumProxies);
	for (int i = 1; i < fNumIterations; i++)
	{
		VSA::fitProxy(faceList, proxyList);
		VSA::flood(faceList, proxyList);
	}

	// Gather Proxy information and assign color to mesh faces
	// Assign proxy label to faces too, using BlindData
	// Need to save ProxyList data structure for future use (probably as a 
	// complex node attribute)
	//
	status = getFloodingResult();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	return status;
}

MStatus isimpFty::buildFaceNeighbors()
{
	MStatus status;
	MFnMesh meshFn(fMesh);

	Size numFaces = meshFn.numPolygons(&status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	faceList.resize(numFaces);

	MItMeshPolygon faceIter(fMesh);

	for (FaceIndex i = 0; i < numFaces; i++)
	{
		VSAFace face;
		status = VSAFace::build(face, faceIter, i);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		faceList[i] = face;
	}
	return MS::kSuccess;
}

MStatus isimpFty::getFloodingResult()
{
	// Gather Proxy information and assign color to mesh faces
	// Assign proxy label to faces too, using BlindData
	//
	MStatus status;
	MFnMesh meshFn(fMesh);

	Size numFaces = meshFn.numPolygons(&status);

	MItMeshPolygon faceIter(fMesh);

	for (FaceIndex i = 0; i < numFaces; i++)
	{
		auto newColor = faceList[i].getColorByLabel();
		status = meshFn.setFaceColor(newColor, i);
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}
	return MS::kSuccess;
}