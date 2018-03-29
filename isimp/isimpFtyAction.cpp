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
#include <maya/MPointArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MTimer.h>

// Function Sets
//
#include <maya/MFnMesh.h>

// Iterators
//
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshEdge.h>

#define MStatusAssert(state,message)				\
		if( !(state) ) {							\
			MString error("Assertion failed: ");	\
			error += message;						\
			MGlobal::displayError(error);			\
			return MS::kFailure;					\
		}

#define MCheckStatus(status,message)											\
	if ( MS::kSuccess != status ) {												\
        MGlobal::displayError(MString(message) + ": " + status.errorString());  \
        return status;															\
    }

#define ErrorReturn(message) MGlobal::displayError(MString(message)); return MS::kFailure;

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
		break; }
	case kGenerate: {
		status = doMeshing();
		break; }
	//case kRefresh: {
	//	status = doReFlooding();
	//	break; }
	//case kAddProxyBySeed: {
	//	status = doAddProxy();
	//	break; }
	//case kDeleteProxyBySeed: {
	//	status = doDelProxy();
	//	break; }
	//case kPaintProxyByFace: {
	//	status = doPaintProxy();
	//	break; }
	default:
		status = MS::kFailure;
		break;
	}

	return status;
}

MStatus isimpFty::doFlooding() {
	MStatus status;
	MTimer timer;

	// Prepare mesh for Distortion Minimizing Flooding
	//
	MFnMesh meshFn(fMesh);

	// Clear proxy labels data stored in mesh
	//if (meshFn.hasBlindData(MFn::kMeshPolygonComponent,
	//	LABEL_BLIND_DATA_ID, &status))
	//{
	//	status = meshFn.clearBlindData(MFn::kMeshPolygonComponent, 
	//		LABEL_BLIND_DATA_ID);
	//}
	//CHECK_MSTATUS_AND_RETURN_IT(status);

	// Start Variational Shape Approximation Routine
	//

	// Build FaceNeighbors data structure
	//
	timer.beginTimer();
	status = buildFaceNeighbors();
	timer.endTimer();
	cout << "Build Neighbors Time " << timer.elapsedTime() << endl;
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Run Distortion Minimizing Flooding on given mesh using attributes
	// such as numProxies, numIterations, etc.
	//
	VSA::init(faceList, proxyList, fNumProxies);
	for (int i = 1; i < fNumIterations; i++)
	{
		timer.beginTimer();
		VSA::fitProxy(faceList, proxyList);
		timer.endTimer();
		cout << "Fit Proxy Time " << timer.elapsedTime() << endl;

		timer.beginTimer();
		VSA::flood(faceList, proxyList);
		timer.endTimer();
		cout << "Flooding Time " << timer.elapsedTime() << endl;
	}

	// Gather Proxy information and assign color to mesh faces
	// Assign proxy label to faces too, using BlindData
	// Need to save ProxyList data structure for future use (probably as a 
	// complex node attribute)
	//
	timer.beginTimer();
	status = getFloodingResult();
	timer.endTimer();
	cout << "Get Result Time " << timer.elapsedTime() << endl;
	CHECK_MSTATUS_AND_RETURN_IT(status);

	return status;
}

MStatus isimpFty::doMeshing()
{
	MStatus status;
	MFnMesh meshFn(fMesh);

	if (false == meshFn.hasBlindData(MFn::kMeshPolygonComponent,
		LABEL_BLIND_DATA_ID, &status))
	{
		ErrorReturn("ERROR getting flooding information");
	}
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Create the new mesh
	int cube_cnts[6] = {
		4,4,4,4,4,4
	};
	int cube_gons[24] = {
		0,3,2,1,
		7,4,5,6,
		2,6,5,1,
		0,4,7,3,
		2,3,7,6,
		1,5,4,0
	};
	
	int numVertices = 8, numPolygons = 6;

	MFloatPointArray	newVertices;

	float a = 1.f;
	newVertices.append(a, a, a); newVertices.append(a, -a, a); 
	newVertices.append(-a, -a, a); newVertices.append(-a, a, a);
	newVertices.append(a, a, -a); newVertices.append(a, -a, -a); 
	newVertices.append(-a, -a, -a); newVertices.append(-a, a, -a);

	MIntArray 	polygonCounts(cube_cnts, 6);
	MIntArray 	polygonConnects(cube_gons, 24);

	MStatusAssert(newVertices.length() == 8, "vertices length error");
	MStatusAssert(polygonCounts.length() == 6, "polygonCounts length error");
	MStatusAssert(polygonConnects.length() == 24, "polygonCounts length error");

	// Note:
	// If the functionset is operating on a mesh node with construction history, 
	// this method will fail as the node will continue to get its geometry from 
	// its history connection.
	// To use this method you must first break the history connection.

	status = meshFn.createInPlace(numVertices, numPolygons, newVertices, polygonCounts, polygonConnects);

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

	status = checkOrCreateBlindDataType();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	for (FaceIndex i = 0; i < numFaces; i++)
	{
		// Gather Proxy information and assign color to mesh faces
		auto newColor = faceList[i].getColorByLabel();
		status = meshFn.setFaceColor(newColor, i);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		// Assign proxy label to faces too, using BlindData
		status = meshFn.setIntBlindData(i, MFn::kMeshPolygonComponent, 
			LABEL_BLIND_DATA_ID, LABEL_BL_SHORT_NAME, faceList[i].label);
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}
	return MS::kSuccess;
}

MStatus isimpFty::checkOrCreateBlindDataType()
{
	// First, make sure that the blind data attribute exists,
	// Otherwise, create it.
	//
	MStatus status;
	MFnMesh meshFn(fMesh);

	if (false == meshFn.isBlindDataTypeUsed(LABEL_BLIND_DATA_ID, &status))
	{
		MStringArray longNames, shortNames, formatNames;

		longNames.append(LABEL_BL_LONG_NAME);
		shortNames.append(LABEL_BL_SHORT_NAME);
		formatNames.append("int");
		status = meshFn.createBlindDataType(
			LABEL_BLIND_DATA_ID, longNames, shortNames, formatNames);
		MCheckStatus(status, "ERROR creating blind data type");
	}
	return status;
}