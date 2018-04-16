//
// Copyright (C) Supernova Studio
//
// File: vsaTypes.h
// Header file for data types used in vsa
//
// MEL Command: isimp
//
// Author: George Zhu
//

#ifndef MAYA_VSA_TYPES_H
#define MAYA_VSA_TYPES_H

// C++ STL Includes
//
#include <set>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <string>  
#include <list>

// MAYA API Includes
//
#include <maya/MVector.h>
#include <maya/MPoint.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>

#define PROXY_BLIND_DATA_ID 15206
#define LABEL_BL_LONG_NAME "proxy_label"
#define LABEL_BL_SHORT_NAME "pxl"
#define SEED_BL_LONG_NAME "seed_array"
#define SEED_BL_SHORT_NAME "seed"

#define PriorityQueue std::multiset
#define Array std::vector
#define Map std::unordered_map
#define newEntry std::make_pair
#define List std::list
#define Set std::unordered_set

typedef int ProxyLabel;
typedef int FaceIndex;
typedef int VertexIndex;
typedef int EdgeIndex;
typedef int Size;
typedef int Index;

typedef MPoint Point3D;
typedef MVector Vector3D;

// Error handling functions
#define ErrorReturn(message) MGlobal::displayError(MString(message)); return MS::kFailure;

#define MCheckStatus(status,message)											\
	if ( MS::kSuccess != status ) {												\
        MGlobal::displayError(MString(message) + ": " + status.errorString());  \
        return status;															\
    }

class MeshingContext
{
public:
	MeshingContext(MObject &meshObj) : meshFn(meshObj), faceIter(meshObj),
		edgeIter(meshObj), vertexIter(meshObj) { }
	// Function sets, pre-built because building them are very expensive
	// SHOULD ONLY USE setIndex() for iterators!!!
	// always setIndex() before use!!!!
	MFnMesh			meshFn;
	MItMeshPolygon	faceIter;
	MItMeshEdge		edgeIter;
	MItMeshVertex	vertexIter;
};


#endif //MAYA_VSA_TYPES_H
