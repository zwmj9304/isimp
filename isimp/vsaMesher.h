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

class VSAMesher
{
public:
	VSAMesher(MObject &mesh) : meshObj(mesh) {}

	MStatus initAnchors(Array<Proxy> &proxyList);
	MStatus refineAnchors(Array<Proxy> &proxyList, double threshold = 2.0);

	MStatus buildNewVerticesList(Array<Proxy> &proxyList, 
								 Map<VertexIndex, VertexIndex> &newIndices, 
								 MFloatPointArray &newVertices,
								 int &numVertices);
	MStatus buildNewFacesList(Array<Proxy> &proxyList,
							  Map<VertexIndex, VertexIndex> &newIndices,
							  MIntArray &polygonCounts, 
							  MIntArray &polygonConnects,
							  int &numPolygons);

private:
	MObject &meshObj;
	Map<VertexIndex, Array<ProxyLabel>> anchorVertices;

	void newAnchor(Array<Proxy> &proxyList, VertexIndex vertex);
	VertexIndex splitEdge(Array<Proxy> &proxyList, Proxy &proxy, 
		VertexIndex v1, VertexIndex v2, double threshold);

	// cross product
	static inline Vector3D cross(const Vector3D& u, const Vector3D& v);
	static inline Vector3D pointOnPlane(Vector3D &point, Vector3D &planeNormal, Vector3D &planeCentroid);

};

#endif