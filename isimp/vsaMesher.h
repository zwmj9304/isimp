//
// Copyright (C) Supernova Studio
//
// File: vsaMesher.h
// Meshing routine for Variational Shape Approximation 
//
// MEL Command: isimp
//
// Author: George Zhu

#ifndef MAYA_VSA_MESHER_H
#define MAYA_VSA_MESHER_H

#include "vsaTypes.h"
#include "vsaProxy.h"
#include "vsaFace.h"
#include <maya/MItMeshEdge.h>


class VSAMesher
{
public:
	VSAMesher(MObject &mesh,
		Array<Proxy> &proxyList,	Array<VSAFace> &faceList,
		MFnMesh &meshFn,			MItMeshPolygon &faceIt, 
		MItMeshEdge &edgeIt,		MItMeshVertex &vertexIt) : 
		meshObj(mesh), proxyList(proxyList), faceList(faceList),
		context(mesh)
	{}

	MStatus initAnchors();
	MStatus refineAnchors(double threshold = 2.0);

	MStatus buildNewVerticesList(Map<VertexIndex, VertexIndex> &newIndices, 
								 MFloatPointArray &newVertices,
								 int &numVertices);
	MStatus buildNewFacesList(Map<VertexIndex, VertexIndex> &newIndices,
							  MIntArray &polygonCounts, 
							  MIntArray &polygonConnects,
							  int &numPolygons);

	static int m_counter[10];
	static double m_time[10];

private:
	MObject &meshObj;
	MeshingContext context;
	Array<Proxy>	&proxyList;
	Array<VSAFace>	&faceList;

	Map<VertexIndex, Array<ProxyLabel>> anchorVertices;

	void newAnchor(VertexIndex vertex);
	VertexIndex splitEdge(Proxy &proxy, 
		VertexIndex v1, VertexIndex v2, double threshold);

	// cross product
	static inline Vector3D cross(const Vector3D& u, const Vector3D& v);
	static inline Vector3D pointOnPlane(Vector3D &point, Vector3D &planeNormal, Vector3D &planeCentroid);

};

#endif