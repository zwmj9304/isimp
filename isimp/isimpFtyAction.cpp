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
	VSAFlooding::init(faceList, proxyList, fNumProxies);
	for (int i = 1; i < fNumIterations; i++)
	{
		timer.beginTimer();
		VSAFlooding::fitProxy(faceList, proxyList);
		timer.endTimer();
		cout << "Fit Proxy Time " << timer.elapsedTime() << endl;

		timer.beginTimer();
		VSAFlooding::flood(faceList, proxyList);
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

	// First, get proxy information from input mesh
	status = rebuildProxyList();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	VSAMesher mesher(fMesh);
	// First run VSA routines to find anchor vertices
	status = mesher.initAnchors(proxyList);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = mesher.refineAnchors(proxyList, edgeSplitThreshold);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Create the new mesh
	int numVertices, numPolygons;
	MFloatPointArray	newVertices;
	MIntArray 	polygonCounts;
	MIntArray 	polygonConnects;
	
	// This maps old vertex indices to new indices
	Map<VertexIndex, VertexIndex> newIndices;
	status = mesher.buildNewVerticesList(proxyList, newIndices, newVertices, numVertices);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = mesher.buildNewFacesList(proxyList, newIndices, polygonCounts, polygonConnects, numPolygons);
	CHECK_MSTATUS_AND_RETURN_IT(status);
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
		ProxyLabel label = faceList[i].label;
		status = meshFn.setIntBlindData(i, MFn::kMeshPolygonComponent, 
			PROXY_BLIND_DATA_ID, LABEL_BL_SHORT_NAME, label);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		bool isSeed = label < 0 ? false : (proxyList[label].seed == i);
		status = meshFn.setBoolBlindData(i, MFn::kMeshPolygonComponent,
			PROXY_BLIND_DATA_ID, SEED_BL_SHORT_NAME, isSeed);
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

	if (false == meshFn.isBlindDataTypeUsed(PROXY_BLIND_DATA_ID, &status))
	{
		MStringArray longNames, shortNames, formatNames;

		longNames.append(LABEL_BL_LONG_NAME);
		shortNames.append(LABEL_BL_SHORT_NAME);
		formatNames.append("int");

		longNames.append(SEED_BL_LONG_NAME);
		shortNames.append(SEED_BL_SHORT_NAME);
		formatNames.append("boolean");
		status = meshFn.createBlindDataType(
			PROXY_BLIND_DATA_ID, longNames, shortNames, formatNames);
		MCheckStatus(status, "ERROR creating blind data type");
	}
	return status;
}

MStatus isimpFty::rebuildProxyList()
{
	MStatus status;
	MFnMesh meshFn(fMesh);
	MItMeshPolygon faceIter(fMesh);

	if (false == meshFn.hasBlindData(MFn::kMeshPolygonComponent,
		PROXY_BLIND_DATA_ID, &status))
	{
		ErrorReturn("ERROR getting proxy information from mesh");
	}
	CHECK_MSTATUS_AND_RETURN_IT(status);

	int highestLabel = 0;
	int numFaces = meshFn.numPolygons();

	// Build a map to record all seed faces
	//
	Map<ProxyLabel, FaceIndex> labelMap;
	for (FaceIndex i = 0; i < numFaces; i++)
	{
		ProxyLabel label;
		bool isSeed;
		status = meshFn.getIntBlindData(i, MFn::kMeshPolygonComponent, 
			PROXY_BLIND_DATA_ID, LABEL_BL_SHORT_NAME, label);
		MCheckStatus(status, "ERROR getting proxy information from mesh");
		highestLabel = label > highestLabel ? label : highestLabel;

		status = meshFn.getBoolBlindData(i, MFn::kMeshPolygonComponent,
			PROXY_BLIND_DATA_ID, SEED_BL_SHORT_NAME, isSeed);
		if (isSeed && label >= 0)
		{
			labelMap.insert(newEntry(label, i));
		}
	}

	if (labelMap.size() == 0)
	{
		ErrorReturn("ERROR proxy information not initialized");
	}

	fNumProxies = highestLabel + 1;
	for (ProxyLabel l = 0; l < fNumProxies; l++)
	{
		Proxy newProxy(l);
		auto labelEntry = labelMap.find(l);
		if (labelEntry != labelMap.end())
		{
			FaceIndex seedFace = labelEntry->second;
			FaceIndex prev;
			faceIter.setIndex(seedFace, prev);
			newProxy.seed = labelEntry->second;
			newProxy.centroid = faceIter.center();
			faceIter.getNormal(newProxy.normal);
		}
		else
		{
			newProxy.valid = false;
		}
		proxyList.push_back(newProxy);
	}

	return status;
}
