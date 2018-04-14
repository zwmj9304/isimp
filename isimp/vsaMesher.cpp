//
// Copyright (C) Supernova Studio
//
// File: vsaMesher.cpp
// Meshing routine for Variational Shape Approximation 
//
// MEL Command: isimp
//
// Author: George Zhu

#include "vsaMesher.h"

#include "maya/MItMeshVertex.h"
#include "maya/MIntArray.h"
#include "maya/MFnMesh.h"
#include "maya/MGlobal.h"
#include "maya/MFloatPointArray.h"

MStatus VSAMesher::initAnchors()
{
	MStatus status;
	int numVertices = context.meshFn.numVertices();
	VertexIndex prev;

	// Iterate through all vertices to find border 
	// halfedges for all proxies
	//
	for (VertexIndex v = 0; v < numVertices; v++)
	{
		Set<ProxyLabel> connectedProxies;
		Array<ProxyLabel> labelMapping;
		// for each vertex, check its neighboring faces
		MIntArray connectedVertices;

		context.vertexIter.setIndex(v, prev);
		context.vertexIter.getConnectedVertices(connectedVertices);

		for (int i = 0; i < (int) connectedVertices.length(); i++)
		{
			HalfEdge h(context, v, connectedVertices[i]);
			if (h.isBoundary()) {
				connectedProxies.insert(-1);
				continue;
			}
			Proxy &p = proxyList[h.faceLabel(faceList)];
			if (p.isBorder(context, faceList, h)) {
				if (connectedProxies.find(p.label) != connectedProxies.end())
				{
					// This proxies has been seen before
					continue;
				}
				labelMapping.push_back(p.label);
				if (false == p.borderHalfEdge.isValid())
				{
					p.borderHalfEdge = h;
				}
				connectedProxies.insert(p.label);
			}
		}
		if (connectedProxies.size() >= 3) {
			anchorVertices.insert(newEntry(v, labelMapping));
		}
	}
	// initialize the sorted anchor list for all proxies
	for (auto &p : proxyList) {
		if (!p.valid) continue;
		// visit all border vertices in order
		Size count = 0;
		auto he = p.borderHalfEdge;
		if (false == he.isValid()) {
			ErrorReturn("Unassigned proxy during meshing init");
		}
		do {
			auto v = he.vertex();
			if (anchorVertices.find(v) != anchorVertices.end()) {
				p.anchors.push_back(v);
			}
			count++;
			he = p.nextHalfEdgeOnBorder(context, faceList, he);
			if (false == he.isValid())
			{
				ErrorReturn("Invalid halfedge walk");
			}
		} while (he != p.borderHalfEdge);
		p.borderEdgeCount = count;
	}
	return MS::kSuccess;
}

MStatus VSAMesher::refineAnchors(double threshold)
{
	MStatus status;
	// first make sure every proxy has 3 or more anchors
	for (auto &p : proxyList) {
		if (!p.valid) continue;
		if (p.anchors.empty()) {
			// let proxy at least have a anchor to start with
			newAnchor(p.borderHalfEdge.vertex());
		}
		if (p.anchors.size() == 1) {
			// add an anchor to the far side of the original anchor
			Size steps = p.borderEdgeCount / 2;
			auto he = p.borderHalfEdge;
			for (Size i = 0; i < steps; i++) {
				he = p.nextHalfEdgeOnBorder(context, faceList, he);
			}
			newAnchor(he.vertex());
		}
		if (p.anchors.size() == 2) {
			// a threshold value of 0.0 means a split must happen regardless of the split criterion
			// but don't recurse split
			auto anchorHe = p.findHalfEdgeOnBorder(context, faceList, p.anchors.front());
			splitEdge(p, anchorHe, p.anchors.back(), -1.0);
		}
	} // end for loop

	// next recursively split each edge
	// TODO each edge is actually checked twice for split, fix by keeping track of split edges
	for (auto &p : proxyList) {
		if (!p.valid) continue;
		auto prev = --p.anchors.end();
		auto next = p.anchors.begin();
		while (next != p.anchors.end()) {
			auto anchorHe = p.findHalfEdgeOnBorder(context, faceList, *prev);
			splitEdge(p, anchorHe, *next, threshold);
			prev = next;
			next++;
		}
	}
	return MS::kSuccess;
}

MStatus VSAMesher::buildNewVerticesList(
	Map<VertexIndex, VertexIndex> &newIndices,
	MFloatPointArray &newVertices,
	int &numVertices)
{
	MStatus status;
	VertexIndex prev;
	VertexIndex i = 0;
	for (auto vpair : anchorVertices)
	{
		VertexIndex v = vpair.first;
		newIndices.insert(newEntry(v, i++));
		auto labels = vpair.second;
		Vector3D avgPosition = Vector3D::zero;
		Size degree = (Size) labels.size();

		context.vertexIter.setIndex(v, prev);
		for (auto l : labels) {
			Vector3D positionVec(context.vertexIter.position());
			avgPosition += pointOnPlane(positionVec, proxyList[l].normal, Vector3D(proxyList[l].centroid));
		}
		avgPosition *= 1.0 / (double)degree;
		newVertices.append(MFloatPoint(avgPosition));
	}
	numVertices = (Size) anchorVertices.size();
	return MS::kSuccess;
}

MStatus VSAMesher::buildNewFacesList(
	Map<VertexIndex, VertexIndex> &newIndices,
	MIntArray &polygonCounts,
	MIntArray &polygonConnects,
	int &numPolygons) 
{
	for (auto &p : proxyList) 
	{
		Size currentFaceCount = (Size) p.anchors.size();
		if (false == p.valid) continue;
		for (auto v : p.anchors)
		{
			VertexIndex currentIndex = newIndices[v];
			polygonConnects.append(currentIndex);
		}
		polygonCounts.append(currentFaceCount);
	}
	numPolygons = polygonCounts.length();
	return MS::kSuccess;
}

void VSAMesher::newAnchor(VertexIndex vertex)
{
	MStatus status;
	Set<ProxyLabel> finishedLabels;
	Array<ProxyLabel> labelMapping;

	// find the border halfedges corresponding to the new anchor
	int prev;
	MIntArray connectedVertices;
	status = context.vertexIter.setIndex(vertex, prev);
	status = context.vertexIter.getConnectedVertices(connectedVertices);

	for (int i = 0; i < (int) connectedVertices.length(); i++)
	{
		HalfEdge he(context, vertex, connectedVertices[i]);
		if (he.isBoundary()) continue;
		ProxyLabel l = he.faceLabel(faceList);
		if (finishedLabels.find(l) == finishedLabels.end())
		{
			proxyList[l].addAnchor(context, faceList, vertex);
			labelMapping.push_back(l);
			finishedLabels.insert(l);
		}
	}
	// add to global anchor map
	anchorVertices.insert(newEntry(vertex, labelMapping));
}

HalfEdge VSAMesher::splitEdge(Proxy & proxy, HalfEdge v1h, VertexIndex v2, double threshold)
{
	MStatus status;
	int prev;
	double largestDistance = 0.0;
	HalfEdge newAnchorHalfEdge;
	//VertexIndex newAnchorVertex;

	// find the vertex on edge that has largest distance to the vector (v1, v2)
	status = context.vertexIter.setIndex(v1h.vertex(), prev);
	Point3D v1p = context.vertexIter.position();
	status	= context.vertexIter.setIndex(v2, prev);
	Point3D v2p = context.vertexIter.position();
	Vector3D v1v2 = v2p - v1p;
	double edgeLength = v1v2.length();
	v1v2.normalize();

	//auto he = proxy.findHalfEdgeOnBorder(context, faceList, v1);
	auto he = v1h;
	he = proxy.nextHalfEdgeOnBorder(context, faceList, he);
	while (he.vertex() != v2) {
		status = context.vertexIter.setIndex(he.vertex(), prev);
		Vector3D vec = context.vertexIter.position() - v1p;
		double dist = cross(vec, v1v2).length();
		if (dist > largestDistance) {
			largestDistance = dist;
			newAnchorHalfEdge = he;
		}
		he = proxy.nextHalfEdgeOnBorder(context, faceList, he);
	}

	if (largestDistance == 0.0)
	{
		// invalid return value
		return HalfEdge();
	}

	// non-recursive split:
	if (threshold < 0) {
		newAnchor(newAnchorHalfEdge.vertex());
		return newAnchorHalfEdge;
	}

	// recursive spilt:
	double sinProxyNormals;
	// calculate split criterion
	auto borderHalfedge = newAnchorHalfEdge;
	if (borderHalfedge.twin().isBoundary()) 
	{ 
		sinProxyNormals = 1.0; // we want an accurate edge
	} 
	else 
	{
		Vector3D N1 = proxy.normal;
		Vector3D N2 = proxyList[borderHalfedge.twin().faceLabel(faceList)].normal;
		sinProxyNormals = cross(N1, N2).length();
	}
	double splitCriterion = largestDistance * sinProxyNormals / edgeLength;
	if (splitCriterion > threshold) {
		VertexIndex newAnchorVertex = newAnchorHalfEdge.vertex();
		newAnchor(newAnchorVertex);
		// recursion
		splitEdge(proxy, v1h, newAnchorVertex, threshold);
		splitEdge(proxy, newAnchorHalfEdge, v2, threshold);
		return newAnchorHalfEdge;
	}
	else 
	{
		// invalid return value
		return HalfEdge();
	}
}

inline Vector3D VSAMesher::cross(const Vector3D & u, const Vector3D & v)
{
	return Vector3D(u.y*v.z - u.z*v.y,
					u.z*v.x - u.x*v.z,
					u.x*v.y - u.y*v.x);
}

inline Vector3D VSAMesher::pointOnPlane(Vector3D & point, Vector3D & planeNormal, Vector3D & planeCentroid)
{
	double d = planeNormal * (point - planeCentroid);
	return point - d * planeNormal;
}
