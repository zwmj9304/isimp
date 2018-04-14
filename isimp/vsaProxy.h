//
// Copyright (C) Supernova Studio
//
// File: vsaProxy.h
// Define APIs to interact 
//
// MEL Command: isimp
//
// Author: George Zhu

#ifndef MAYA_VSA_PROXY_H
#define MAYA_VSA_PROXY_H

#include "vsaTypes.h"
#include "vsaFace.h"
#include "maya/MObject.h"

// Simulate the behavior of a HalfEdge based mesh structure
//
class HalfEdge
{
public:
	// Constructor for invalid halfedge
	//
	HalfEdge() { begin = -1; end = -1; ctx = nullptr; }
	~HalfEdge() {}

	// New constructor
	HalfEdge(MeshingContext &context, VertexIndex begin, VertexIndex end);
	HalfEdge(MeshingContext *ctx, VertexIndex begin, VertexIndex end);

	// Copy constructor
	HalfEdge(const HalfEdge &he) { *this = he; }

	static HalfEdge fromFace(MeshingContext &context, FaceIndex faceIdx);
	static HalfEdge fromEdge(MeshingContext &context, EdgeIndex edgeIdx,
		VertexIndex startIdx=-1);
	static HalfEdge fromVertex(MeshingContext &context, VertexIndex vertIdx);

	HalfEdge next() const;
	HalfEdge prev() const;
	HalfEdge twin() const;

	FaceIndex	face() const;
	EdgeIndex	edge() const;
	VertexIndex vertex() const;
	ProxyLabel	faceLabel(Array<VSAFace> &faceList) const;

	bool isBoundary() const;
	bool isValid() const;

	bool operator==(const HalfEdge &other) const;
	bool operator!=(const HalfEdge &other) const;
	HalfEdge& operator=(const HalfEdge &other);

private:
	VertexIndex		begin;
	VertexIndex		end;
	MeshingContext	*ctx;
};


class BorderRing {
public:
	BorderRing()
	{
		borderHalfEdge = HalfEdge();
		borderEdgeCount = -1;
	}

	BorderRing(HalfEdge borderHalfEdge, Size borderEdgeCount)
	{
		this->borderHalfEdge = borderHalfEdge;
		this->borderEdgeCount = borderEdgeCount;
	}

	// Used by meshing routines, each ring reprensents a one closed boundary
	// Holes in polygons are supported in this way
	//
	HalfEdge borderHalfEdge;
	Size borderEdgeCount;
	List<VertexIndex> anchors; ///< anchors must be sorted by the same order of halfedge
};


class Proxy {
public:
	Proxy()
	{ 
		this->valid = true;
		this->label = -1;
		this->seed = -1;
		this->normal = Vector3D::zero;
		this->centroid = Point3D::origin;
		this->borderEdgeCount = -1;
		this->borderHalfEdge = HalfEdge();
	}
	Proxy(ProxyLabel label)
	{ 
		this->valid = true;
		this->label = label;
		this->seed = -1;
		this->normal = Vector3D::zero;
		this->centroid = Point3D::origin;
		this->borderHalfEdge = HalfEdge();
		this->borderEdgeCount = -1;
	}

	Proxy& operator=(const Proxy &other);

	/** The normal of the proxy */
	Vector3D	normal;
	/** The centroid of the proxy */
	Point3D		centroid;
	/**
	* The seed of a proxy is the face from which a region can be grown.
	* It is updated every iteration using Lloyd algorithm.
	*/
	FaceIndex	seed;
	/** Whether this proxy is deleted */
	bool valid;

	ProxyLabel label;

	HalfEdge borderHalfEdge;
	Size borderEdgeCount;
	List<HalfEdge> anchors;

	// Used by meshing routines, each ring reprensents a one closed boundary
	// Holes in polygons are supported in this way
	//
	// Note:
	// The first border ring in the list indicates the outer border of this polygon
	// The remaining rings are essentially holes in this polygon
	//
	//List<BorderRing> borderRings;
	//Set<HalfEdge> borderHalfEdges;

	// For a given border halfedge on this proxy, find the next halfedge on border
	HalfEdge nextHalfEdgeOnBorder(MeshingContext &context, Array<VSAFace> &faceList, const HalfEdge &he);

	// Given a boarder vertex of this proxy, find the outgoing halfedge on border
	HalfEdge findHalfEdgeOnBorder(MeshingContext &context, Array<VSAFace> &faceList, VertexIndex v);

	// Determine whether a given halfedge is on the border of this proxy
	bool isBorder(MeshingContext &context, Array<VSAFace> &faceList, const HalfEdge &he) const;

	// Add an anchor vertex to this proxy. After adding, anchor vector should remain sorted.
	// Requires that this vertex is on the border of this proxy.
	void addAnchor(MeshingContext &context, Array<VSAFace> &faceList, HalfEdge newAnchorHe);

	// Add a new border ring to this proxy
	// The return value indicates whether addRing succeeded
	bool addRing(HalfEdge borderHalfEdge);
};


#endif //MAYA_VSA_PROXY_H