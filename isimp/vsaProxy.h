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
#include "maya/MObject.h"
// Simulate the behavior of a HalfEdge based mesh structure
//
class HalfEdge
{
public:
	// Constructor for invalid halfedge
	//
	HalfEdge() { begin = -1; end = -1; meshPtr = nullptr; }
	~HalfEdge() {}

	// New constructor
	HalfEdge(MObject &meshObj, VertexIndex begin, VertexIndex end);
	HalfEdge(MObject *meshPtr, VertexIndex begin, VertexIndex end);
	// Copy constructor
	HalfEdge(const HalfEdge &he) { begin = he.begin; end = he.end; meshPtr = he.meshPtr; }

	static HalfEdge fromFace(MObject &meshObj, FaceIndex faceIdx);
	static HalfEdge fromEdge(MObject &meshObj, EdgeIndex edgeIdx, 
		VertexIndex startIdx=-1);
	static HalfEdge fromVertex(MObject &meshObj, VertexIndex vertIdx);

	HalfEdge next() const;
	HalfEdge twin() const;

	FaceIndex	face() const;
	EdgeIndex	edge() const;
	VertexIndex vertex() const;
	ProxyLabel	faceLabel() const;

	bool isBoundary() const;
	bool isValid() const;

	bool operator==(const HalfEdge &other) const;
	bool operator!=(const HalfEdge &other) const;
	HalfEdge& operator=(const HalfEdge &other);

private:
	VertexIndex		begin;
	VertexIndex		end;
	MObject*		meshPtr;
};


class Proxy {
public:
	Proxy()
	{ 
		this->valid = true;
		this->label = -1;
		this->seed = -1;
		this->borderEdgeCount = -1;
		this->normal = Vector3D::zero;
		this->centroid = Point3D::origin;
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

	/** Used by meshing routines */
	HalfEdge borderHalfEdge;
	Size borderEdgeCount;
	List<VertexIndex> anchors; ///< anchors must be sorted by the same order of halfedge

	
	// For a given border halfedge on this proxy, find the next halfedge on border
	HalfEdge nextHalfEdgeOnBorder(MObject &mesh, const HalfEdge &he);

	// Given a boarder vertex of this proxy, find the outgoing halfedge on border
	HalfEdge findHalfEdgeOnBorder(MObject &mesh, VertexIndex v);

	// Determine whether a given halfedge is on the border of this proxy
	bool isBorder(MObject &mesh, const HalfEdge &he) const;

	// Add an anchor vertex to this proxy. After adding, anchor vector should remain sorted.
	// Requires that this vertex is on the border of this proxy.
	void addAnchor(MObject &mesh, VertexIndex newAnchor);
};


#endif //MAYA_VSA_PROXY_H