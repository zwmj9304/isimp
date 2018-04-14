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
		cout << "[iSimp] Command 0: Flood" << endl;
		status = doFlooding();
		break; }
	case kGenerate: {
		cout << "[iSimp] Command 1: Generate Mesh" << endl;
		status = doMeshing();
		break; }
	case kAddProxyBySeed: {
		cout << "[iSimp] Command 2: Add Region" << endl;
		status = doAddProxy();
		break; }
	case kDeleteProxyBySeed: {
		cout << "[iSimp] Command 3: Delete Region" << endl;
		status = doDelProxy();
		break; }
	//case kPaintProxyByFace: {
	//	status = doPaintProxy();
	//	break; }
	//case kRefresh: {
	//	status = doReFlooding();
	//	break; }
	default:
		status = MS::kFailure;
		break;
	}

	// free resources when finished
	faceList.clear();
	proxyList.clear();

	return status;
}

MStatus isimpFty::doFlooding() {
	MStatus status;
	MTimer timer;
	double floodTime = 0, fitTime = 0;
	// Prepare mesh for Distortion Minimizing Flooding
	//
	MFnMesh meshFn(fMesh);

	// Start Variational Shape Approximation Routine
	//

	// Build FaceNeighbors data structure
	//
	timer.beginTimer();
	status = buildFaceNeighbors();
	timer.endTimer();
	cout << "[iSimp] Build Neighbors Time " << timer.elapsedTime() << "s" << endl;
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Run Distortion Minimizing Flooding on given mesh using attributes
	// such as numProxies, numIterations, etc.
	//
	VSAFlooding::init(faceList, proxyList, fNumProxies);

	timer.beginTimer();
	VSAFlooding::flood(faceList, proxyList);
	timer.endTimer(); floodTime += timer.elapsedTime();

	for (int i = 1; i < fNumIterations; i++)
	{
		timer.beginTimer();
		VSAFlooding::fitProxy(faceList, proxyList);
		VSAFlooding::clear(faceList);
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
	CHECK_MSTATUS_AND_RETURN_IT(status);

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
	status = mesher.initAnchors();
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = mesher.refineAnchors(edgeSplitThreshold);
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
	// Note:
	// If the functionset is operating on a mesh node with construction history, 
	// this method will fail as the node will continue to get its geometry from 
	// its history connection.
	// To use this method you must first break the history connection.
	status = meshFn.createInPlace(numVertices, numPolygons, newVertices, polygonCounts, polygonConnects);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = meshFn.updateSurface();

	timer.endTimer();
	cout << "[iSimp] Rebuild Lists Time   " << timer.elapsedTime() << "s" << endl;
	cout << "[iSimp] Meshing              " << timer.elapsedTime() << "s" << endl;
	return status;
}

MStatus isimpFty::doAddProxy()
{
	MStatus status;

	// First, get proxy information from input mesh
	status = rebuildLists();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Next, build face neighbors for flooding
	status = buildFaceNeighbors();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Get the proxy label of selected face
	if (fComponentIDs.length() == 0)
	{
		// should not happen
		ErrorReturn("No face selected");
	}
	MFnMesh meshFn(fMesh);
	ProxyLabel label;
	FaceIndex f = fComponentIDs[0];
	status = meshFn.getIntBlindData(f, MFn::kMeshPolygonComponent,
		PROXY_BLIND_DATA_ID, LABEL_BL_SHORT_NAME, label);
	MCheckStatus(status, "getting region information");

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

	// Next, build face neighbors for flooding
	status = buildFaceNeighbors();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Get the proxy label of selected face
	if (fComponentIDs.length() == 0)
	{
		// should not happen
		ErrorReturn("No face selected");
	}
	MFnMesh meshFn(fMesh);
	ProxyLabel label;
	FaceIndex f = fComponentIDs[0];
	status = meshFn.getIntBlindData(f, MFn::kMeshPolygonComponent,
		PROXY_BLIND_DATA_ID, LABEL_BL_SHORT_NAME, label);
	MCheckStatus(status, "getting region information");

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
	// Temporary used for performance test
	MStatus status;

	MFnMesh meshFn(fMesh);
	MItMeshPolygon faceIter(fMesh);
	MItMeshVertex vertexIter(fMesh);
	MItMeshEdge edgeIter(fMesh);
	MIntArray intArray;
	int numCalls = 0;
	MTimer timer;
	timer.beginTimer();
	for (int i = 0; i < 10; i++)
	{
		for (faceIter.reset(); !faceIter.isDone(); faceIter.next())
		{
			status = faceIter.getConnectedFaces(intArray);
			numCalls++;
		}
	}
	timer.endTimer();
	double faceTime = timer.elapsedTime();
	cout << numCalls << " f-f connection calls " << faceTime << "s" << endl;

	numCalls = 0;
	timer.beginTimer();
	for (int i = 0; i < 10; i++)
	{
		for (vertexIter.reset(); !vertexIter.isDone(); vertexIter.next())
		{
			status = vertexIter.getConnectedFaces(intArray);
			numCalls++;
		}
	}
	timer.endTimer();
	double vertTime = timer.elapsedTime();
	cout << numCalls << " v-f connection calls " << vertTime << "s" << endl;

	numCalls = 0;
	timer.beginTimer();
	for (int i = 0; i < 10; i++)
	{
		for (edgeIter.reset(); !edgeIter.isDone(); edgeIter.next())
		{
			int num = edgeIter.getConnectedFaces(intArray);
			numCalls++;
		}
	}
	timer.endTimer();
	double edgeTime = timer.elapsedTime();
	cout << numCalls << " e-f connection calls " << edgeTime << "s" << endl;

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

		// Assign proxy label to faces too, using BlindData
		ProxyLabel label = faceList[i].label;
		status = meshFn.setIntBlindData(i, MFn::kMeshPolygonComponent, 
			PROXY_BLIND_DATA_ID, LABEL_BL_SHORT_NAME, label);
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
	MIntArray polygonIDs, proxyLabels;

	status = meshFn.getIntBlindData(MFn::kMeshPolygonComponent,
		PROXY_BLIND_DATA_ID, LABEL_BL_SHORT_NAME, polygonIDs, proxyLabels);
	MCheckStatus(status, "getting proxy information failed");
	if (polygonIDs.length() != meshFn.numPolygons())
	{
		ErrorReturn("not all faces were assigned proxy information");
	}

	int highestLabel = -1;
	int numFaces = meshFn.numPolygons();
	int prev;
	faceList.clear();
	faceList.resize(numFaces);

	// Build a map to record all seed faces
	//
	Map<ProxyLabel, FaceIndex> labelMap;

	for (int id = 0; id < numFaces; id++)
	{
		FaceIndex i = polygonIDs[id];
		ProxyLabel label = proxyLabels[id];
		status = faceIter.setIndex(i, prev);
		MCheckStatus(status, "non-contiguous polygon indexing");

		highestLabel = label > highestLabel ? label : highestLabel;
		// rebuild faceList with label information
		VSAFace face(label);
		status = VSAFace::build(face, faceIter, i);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		faceList[i] = face;

		if (label >= 0 && labelMap.find(label) == labelMap.end())
		{
			labelMap.insert(newEntry(label, i));
		}
	}

	if (labelMap.size() == 0)
	{
		ErrorReturn("proxy information not initialized");
	}

	fNumProxies = highestLabel + 1;
	proxyList.clear();

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
	
	// Recalculate normal, centroid and seed for each proxy
	VSAFlooding::fitProxy(faceList, proxyList);

	timer.endTimer();
	cout << "[iSimp] Rebuild Lists Time   " << timer.elapsedTime() << "s" << endl;

	return status;
}
