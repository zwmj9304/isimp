//
// Copyright (C) Supernova Studio
//
// File: vsaFace.h
// Define APIs to interact with adjacency-list based face data structure
//
// MEL Command: isimp
//
// Author: George Zhu

#include "vsaFace.h"

VSAFace::VSAFace()
{
	label = -1;
	isBoundary = false;
}

MStatus VSAFace::build(VSAFace &face, MItMeshPolygon &faceIter, FaceIndex idx)
{
	MStatus status;
	FaceIndex dummyIndex;
	int triCount;

	status = faceIter.setIndex(idx, dummyIndex);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	status = faceIter.numTriangles(triCount);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	if (triCount != 1)
	{
		// TODO this warning should have a more decent display style
		cerr << "Face " << idx << " is not triangle, try triangulate first" << endl;
		return MS::kFailure;
	}

	// Set neighboring faces
	//
	MIntArray faceNeighbors;
	faceIter.getConnectedFaces(faceNeighbors);
	for (int i = 0; i < 3; ++i)
	{
		if ((int) faceNeighbors.length() > i)
			face.neighbors[i] = faceNeighbors[i];
		else
			face.neighbors[i] = -1;
	}

	status = faceIter.getArea(face.area);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = faceIter.getNormal(face.normal);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	face.normal.normalize();
	face.centroid = faceIter.center(MSpace::kObject, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	return MS::kSuccess;
}

MColor VSAFace::getColorByLabel()
{
	// A pseudo-random color generator
	// TODO: This function should be computed only once for each proxy
	//
	if (label < 0) return MColor(0, 0, 0);
	size_t hashVal = 3048260799447986477L * label + 
					 1110414738616293511L;
	int mask = (1 << 16) - 1;
	int base = 1 << 16;
	float r, g, b;
	r = (float)(hashVal		& mask) / (float)base;
	g = (float)((hashVal >> 16) & mask) / (float)base;
	b = (float)((hashVal >> 32) & mask) / (float)base;
	return MColor(r, g, b);
}