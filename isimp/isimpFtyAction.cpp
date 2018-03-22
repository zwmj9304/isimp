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
		status = doFlooding(meshFn);
		CHECK_STATUS(status);
		break; }
	case kGenerate: {
		status = doMeshing(meshFn);
		CHECK_STATUS(status);
		break; }
	case kRefresh: {
		status = doReFlooding(meshFn);
		CHECK_STATUS(status);
		break; }
	case kAddProxyBySeed: {
		status = doAddProxy(meshFn);
		CHECK_STATUS(status);
		break; }
	case kDeleteProxyBySeed: {
		status = doDelProxy(meshFn);
		CHECK_STATUS(status);
		break; }
	case kPaintProxyByFace: {
		status = doPaintProxy(meshFn);
		CHECK_STATUS(status);
		break; }
	}

	return status;
}

MStatus isimpFty::doFlooding(MFnMesh& meshFn) {
	MStatus status;

	// Prepare mesh for Distortion Minimizing Flooding
	//
	status = meshFn.clearColors();
	// TODO clear proxy labels data stored in mesh

	// Start Variational Shape Approximation Routine
	//

	// Build FaceNeighbors data structure, maybe cache it?
	//

	// Run Distortion Minimizing Flooding on given mesh using attributes
	// such as numProxies, numIterations, etc.
	//

	// Gather Proxy information and assign color to mesh faces
	// Assign proxy label to faces too, using BlindData
	// Need to save ProxyList data structure for future use (probably as a 
	// complex node attribute)
	//


	MColor newColor(0.5f, 0.3f, 0.f);
	status = meshFn.setFaceColor(newColor, 0);

	return status;
}