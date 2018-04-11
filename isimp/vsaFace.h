//
// Copyright (C) Supernova Studio
//
// File: vsaFace.h
// Define APIs to interact with adjacency-list based face data structure
//
// MEL Command: isimp
//
// Author: George Zhu

#ifndef MAYA_VSA_FACE_H
#define MAYA_VSA_FACE_H

#include "vsaTypes.h"

#include <maya/MItMeshPolygon.h>
#include <maya/MObject.h>
#include <maya/MIntArray.h>
#include <maya/MFnMesh.h>

class VSAFace {
public:
	VSAFace();
	VSAFace(ProxyLabel label);
	~VSAFace() {}

	static MStatus	build(VSAFace &face, MItMeshPolygon &faceIter, FaceIndex idx);
	
	MColor		getColorByLabel();

	Point3D		centroid;
	Vector3D	normal;
	FaceIndex	neighbors[3];	///< if a face is on the boundary, its neighbor 
								///< index should be -1
	double		area;			///< this value should be pre-calculated
	FaceIndex	index;			///< the index of this face (for debug)
	ProxyLabel	label;			///< defaults to -1
	bool		isBoundary;		///< dummy face that represents an open boundary

};

#endif
