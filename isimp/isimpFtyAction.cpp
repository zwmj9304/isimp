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
#include <maya/MItMeshVertex.h>
#include <maya/MItMeshEdge.h>

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
		cout << "[iSimp] ---- Command 0: Flood" << endl;
		status = doFlooding();
		break; }
	case kGenerate: {
		cout << "[iSimp] ---- Command 1: Generate Mesh" << endl;
		status = doMeshing();
		break; }
	case kAddProxyBySeed: {
		cout << "[iSimp] ---- Command 2: Add Region" << endl;
		status = doAddProxy();
		break; }
	case kDeleteProxyBySeed: {
		cout << "[iSimp] ---- Command 3: Delete Region" << endl;
		status = doDelProxy();
		break; }
	case kPaintProxyByFace: {
		cout << "[iSimp] ---- Command 4: Turn On Color Display" << endl;
		status = doPaintProxy();
		break; }
	case kRefresh: {
		cout << "[iSimp] ---- Command 5: Refresh" << endl;
		status = doReFlooding();
		break; }
	default:
		status = MS::kFailure;
		break;
	}

	// free resources when finished
	faceList.clear();
	faceList.shrink_to_fit();
	proxyList.clear();
	proxyList.shrink_to_fit();

	return status;
}

MStatus isimpFty::doFlooding() {
	MStatus status;
	MTimer timer;

	// Prepare mesh for Distortion Minimizing Flooding
	// Build FaceNeighbors data structure
	//
	timer.beginTimer();
	status = buildFaceNeighbors();
	timer.endTimer();
	cout << "[iSimp] Build Neighbors Time " << timer.elapsedTime() << "s" << endl;
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Start Variational Shape Approximation Routine
	// Run Distortion Minimizing Flooding on given mesh using attributes
	// such as numProxies, numIterations, etc.
	//
	VSAFlooding::init(faceList, proxyList, fNumProxies);

	status = runVSAIterations();
	return status;
}

MStatus isimpFty::doMeshing()
{
	MStatus status;
	MTimer timer;

	// First, get faceNeighbor and proxy information from input mesh
	status = rebuildLists();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	timer.beginTimer();

	MFnMesh meshFn(fMesh);
	MItMeshEdge edgeIt(fMesh);
	MItMeshPolygon faceIt(fMesh);
	MItMeshVertex vertIt(fMesh);

	VSAMesher mesher(fMesh, proxyList, faceList, meshFn, faceIt, edgeIt, vertIt);
	// First run VSA routines to find anchor vertices
	status = mesher.initAnchors(fKeepHoles);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = mesher.refineAnchors(fKeepHoles, fSplitThreshold);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Create the new mesh
	int numVertices, numPolygons;
	MFloatPointArray	newVertices;
	MIntArray 	polygonCounts;
	MIntArray 	polygonConnects;
	
	// This maps old vertex indices to new indices
	Map<VertexIndex, VertexIndex> newIndices;
	status = mesher.buildNewVerticesList(newIndices, newVertices, numVertices);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = mesher.buildNewFacesList(newIndices, polygonCounts, polygonConnects, numPolygons);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MObject newMeshGeom = meshFn.create(numVertices, numPolygons, newVertices, polygonCounts, polygonConnects, fMesh, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	
	if (fKeepHoles)
	{
		status = mesher.addHoles(meshFn, newIndices, newVertices);
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}

	timer.endTimer();
	cout << "[iSimp] Meshing Time         " << timer.elapsedTime() << "s" << endl;
	return status;
}

MStatus isimpFty::doAddProxy()
{
	MStatus status;

	// First, get proxy information from input mesh
	status = rebuildLists();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Next, Get the proxy label of selected face
	if (fComponentIDs.length() == 0)
	{
		// should not happen
		ErrorReturn("No face selected");
	}
	MFnMesh meshFn(fMesh);
	FaceIndex f = fComponentIDs[0];
	ProxyLabel label = faceList[f].label;

	if (proxyList[label].seed == f) {
		// TODO find around neighbouring faces for a non-seed face
		ErrorReturn("Sorry, try select another face nearby");
	}

	// Add a new proxy to proxyList
	Proxy newProxy(fNumProxies);
	newProxy.seed = f;
	newProxy.centroid = faceList[f].centroid;
	newProxy.normal = faceList[f].normal;

	proxyList.push_back(newProxy);
	fNumProxies++;

	// Redo the flooding
	VSAFlooding::flood(faceList, proxyList);
	// Write proxy info and color info back to model
	status = getFloodingResult();

	return status;
}

MStatus isimpFty::doDelProxy()
{
	MStatus status;

	// First, get proxy information from input mesh
	status = rebuildLists();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Get the proxy label of selected face
	if (fComponentIDs.length() == 0)
	{
		// should not happen
		ErrorReturn("No face selected");
	}
	MFnMesh meshFn(fMesh);
	FaceIndex f = fComponentIDs[0];
	ProxyLabel label = faceList[f].label;

	// Do the actual deletion
	proxyList[label].valid = false;

	// Redo the flooding
	VSAFlooding::flood(faceList, proxyList);
	// Write proxy info and color info back to model
	status = getFloodingResult();

	return status;
}

MStatus isimpFty::doPaintProxy()
{
	// Use this command to turn on color display in undo cases
	return MS::kSuccess;
}

MStatus isimpFty::doReFlooding()
{
	MStatus status;

	// First, get proxy information from input mesh
	status = rebuildLists();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	status = runVSAIterations();
	return status;
}

MStatus isimpFty::runVSAIterations()
{
	MStatus status;
	MTimer timer;
	double floodTime = 0, fitTime = 0;

	timer.beginTimer();
	VSAFlooding::flood(faceList, proxyList);
	timer.endTimer(); floodTime += timer.elapsedTime();

	for (int i = 1; i < fNumIterations; i++)
	{
		timer.beginTimer();
		VSAFlooding::fitProxy(faceList, proxyList);
		timer.endTimer(); fitTime += timer.elapsedTime();

		timer.beginTimer();
		VSAFlooding::flood(faceList, proxyList);
		timer.endTimer();
		timer.endTimer(); floodTime += timer.elapsedTime();
	}

	cout << "[iSimp] Flooding Time:  " << floodTime << "s" << endl;
	cout << "[iSimp] Fit Proxy Time: " << fitTime << "s" << endl;

	// Gather Proxy information and assign color to mesh faces
	// Assign proxy label to faces too, using BlindData
	// Need to save ProxyList data structure for future use (probably as a 
	// complex node attribute)
	//
	timer.beginTimer();
	status = getFloodingResult();
	timer.endTimer();
	cout << "[iSimp] Get Result Time " << timer.elapsedTime() << "s" << endl;

	return status;
}

MStatus isimpFty::buildFaceNeighbors()
{
	MStatus status;
	MFnMesh meshFn(fMesh);

	Size numFaces = meshFn.numPolygons(&status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	cout << "[iSimp] Mesh has " << numFaces << " faces" << endl;

	faceList.clear();
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
	}

	// Encode proxy list as binary string
	Array<FaceIndex> seedArray(fNumProxies);
	for (ProxyLabel p = 0; p < fNumProxies; p++)
	{
		if (proxyList[p].valid)
			seedArray[p] = proxyList[p].seed;
		else
			seedArray[p] = -1;
	}
	// Use the Polygon at index 0 to store all proxy information
	MString binaryData((char *) &seedArray[0], 4 * fNumProxies);
	//if (meshFn.hasBlindDataComponentId(0, MFn::kMeshPolygonComponent, PROXY_BLIND_DATA_ID))
	//{
	//	status = meshFn.clearBlindData(0, MFn::kMeshPolygonComponent, PROXY_BLIND_DATA_ID);
	//	MCheckStatus(status, "cannot clear previous proxy data");
	//}
	status = meshFn.setBinaryBlindData(0, MFn::kMeshPolygonComponent,
		PROXY_BLIND_DATA_ID, SEED_BL_SHORT_NAME, binaryData);
	MCheckStatus(status, "cannot output proxy data");

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

		longNames.append(SEED_BL_LONG_NAME);
		shortNames.append(SEED_BL_SHORT_NAME);
		formatNames.append("binary");

		status = meshFn.createBlindDataType(
			PROXY_BLIND_DATA_ID, longNames, shortNames, formatNames);
		MCheckStatus(status, "creating blind data type");
	}
	return status;
}

MStatus isimpFty::rebuildLists()
{
	MTimer timer;
	timer.beginTimer();

	MStatus status;
	MFnMesh meshFn(fMesh);
	MItMeshPolygon faceIter(fMesh);

	status = buildFaceNeighbors();
	MCheckStatus(status, "failed to rebuild face neighbors");

	// Use the Polygon at index 0 to store all proxy information
	MString seedArrayBinary;
	status = meshFn.getBinaryBlindData(0, MFn::kMeshPolygonComponent,
		PROXY_BLIND_DATA_ID, SEED_BL_SHORT_NAME, seedArrayBinary);
	MCheckStatus(status, "proxy information not initialized");

	int numProxies;
	int *seedArray = (int *)seedArrayBinary.asChar(numProxies);
	numProxies /= 4; // int is four bytes, char is one byte

	if (numProxies <= 0)
	{
		ErrorReturn("zero proxy is found");
	}

	fNumProxies = numProxies;
	proxyList.clear();

	for (ProxyLabel l = 0; l < fNumProxies; l++)
	{
		Proxy newProxy(l);
		FaceIndex seedFace = seedArray[l];
		if (seedFace >= 0)
		{
			FaceIndex prev;
			faceIter.setIndex(seedFace, prev);
			newProxy.seed = seedFace;
			newProxy.centroid = faceIter.center();
			faceIter.getNormal(newProxy.normal);
		}
		else
		{
			newProxy.valid = false;
		}
		proxyList.push_back(newProxy);
	}
	
	// Recalculate normal, centroid and seed for each proxy
	VSAFlooding::flood(faceList, proxyList);
	// VSAFlooding::fitProxy(faceList, proxyList);

	timer.endTimer();
	cout << "[iSimp] Rebuild Lists Time   " << timer.elapsedTime() << "s" << endl;

	return status;
}

MStatus isimpFty::clearVSAData(MFnMesh & meshFn)
{
	MStatus status;
	MIntArray faceList;
	int numFaces = meshFn.numPolygons();
	
	for (int i = 0; i < numFaces; i++)
	{
		faceList.append(i);
	}

	status = meshFn.removeFaceColors(faceList);
	return status;
}
