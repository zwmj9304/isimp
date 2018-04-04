//
// Copyright (C) Supernova Studio
//
// File: vsaProxy.cpp
// Define APIs to interact 
//
// MEL Command: isimp
//
// Author: George Zhu

#include "vsaProxy.h"

#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshVertex.h>
#include <maya/MIntArray.h>


HalfEdge::HalfEdge(MObject &meshObj, VertexIndex begin, VertexIndex end)
{
	this->begin		= begin;
	this->end		= end;
	this->meshPtr	= &meshObj;
}

HalfEdge::HalfEdge(MObject * meshPtr, VertexIndex begin, VertexIndex end)
{
	this->begin = begin;
	this->end = end;
	this->meshPtr = meshPtr;
}

HalfEdge HalfEdge::fromFace(MObject &meshObj, FaceIndex faceIdx)
{
	MStatus status;
	MFnMesh meshFn(meshObj);

	MIntArray vertexList;
	status = meshFn.getPolygonVertices(faceIdx, vertexList);
	if (status != MS::kSuccess || vertexList.length() < 3)
	{
		return HalfEdge();
	}
	return HalfEdge(meshObj, vertexList[0], vertexList[1]);
}

HalfEdge HalfEdge::fromEdge(MObject & meshObj, EdgeIndex edgeIdx, 
	VertexIndex startIdx)
{
	MStatus status;
	MFnMesh meshFn(meshObj);
	int2 vertexList;

	status = meshFn.getEdgeVertices(edgeIdx, vertexList);
	if (status != MS::kSuccess)
	{
		return HalfEdge();
	}
	if (startIdx < 0 || vertexList[0] == startIdx)
	{
		return HalfEdge(meshObj, vertexList[0], vertexList[1]);
	}
	else
	{
		return HalfEdge(meshObj, vertexList[1], vertexList[0]);
	}
}

HalfEdge HalfEdge::fromVertex(MObject & meshObj, VertexIndex vertIdx)
{
	MStatus status;
	MFnMesh meshFn(meshObj);
	MItMeshVertex vertexIter(meshObj);

	MIntArray edgeList;
	int prevIdx;
	status = vertexIter.setIndex(vertIdx, prevIdx);
	status = vertexIter.getConnectedEdges(edgeList);

	if (status != MS::kSuccess || edgeList.length() < 1)
	{
		return HalfEdge();
	}
	return fromEdge(meshObj, edgeList[0], vertIdx);
}

HalfEdge HalfEdge::next() const
{
	FaceIndex faceIdx = face();
	if (faceIdx < 0)
	{
		// TODO add support on border halfedges
		return HalfEdge();
	}

	MStatus status;
	MFnMesh meshFn(*meshPtr);

	MIntArray vertexList;
	status = meshFn.getPolygonVertices(faceIdx, vertexList);
	int degree = vertexList.length();
	for (VertexIndex v = 0; v < degree; v++)
	{
		if (vertexList[v] == end)
		{
			return HalfEdge(meshPtr, end, vertexList[(v + 1) % degree]);
		}
	}
	// Should not reach here
	//
	return HalfEdge();
}

HalfEdge HalfEdge::twin() const
{
	return HalfEdge(meshPtr, end, begin);
}

FaceIndex HalfEdge::face() const
{
	MStatus status;
	MFnMesh meshFn(*meshPtr);
	MItMeshVertex vertexIter(*meshPtr);

	MIntArray faceList;
	int prevIdx;
	status = vertexIter.setIndex(begin, prevIdx);
	status = vertexIter.getConnectedFaces(faceList);

	for (FaceIndex i = 0; i < (int) faceList.length(); i++)
	{
		MIntArray faceVertices;
		status = meshFn.getPolygonVertices(faceList[i], faceVertices);
		int degree = faceVertices.length();
		for (VertexIndex v = 0; v < degree; v++)
		{
			if (faceVertices[v]					== begin &&
				faceVertices[(v + 1) % degree]	== end)
			{
				return faceList[i];
			}
		}
	}
	// This means this halfedge is an border edge
	//
	return -1;
}

EdgeIndex HalfEdge::edge() const
{
	MStatus status;
	MFnMesh meshFn(*meshPtr);
	MItMeshVertex vertexIter(*meshPtr);

	MIntArray edgeList;
	int prevIdx;
	status = vertexIter.setIndex(begin, prevIdx);
	status = vertexIter.getConnectedEdges(edgeList);
	for (EdgeIndex i = 0; i < (int) edgeList.length(); i++)
	{
		int2 edgeVertices;
		status = meshFn.getEdgeVertices(edgeList[i], edgeVertices);
		if (edgeVertices[0] == end || edgeVertices[1] == end)
		{
			return edgeList[i];
		}
	}
	// Should not reach here
	//
	return -1;
}

VertexIndex HalfEdge::vertex() const
{
	return begin;
}

ProxyLabel HalfEdge::faceLabel() const
{
	FaceIndex f = face();
	if (f < 0) return -1;
	MFnMesh meshFn(*meshPtr);
	ProxyLabel label;
	meshFn.getIntBlindData(f, MFn::kMeshPolygonComponent,
		PROXY_BLIND_DATA_ID, LABEL_BL_SHORT_NAME, label);
	return label;
}


bool HalfEdge::isBoundary() const
{
	return face() == -1;
}

bool HalfEdge::isValid() const
{
	return begin != end;
}

bool HalfEdge::operator==(const HalfEdge &other) const
{
	return begin == other.begin && end == other.end;
}

bool HalfEdge::operator!=(const HalfEdge &other) const
{
	return !(*this == other);
}

HalfEdge & HalfEdge::operator=(const HalfEdge & other)
{
	this->begin = other.begin;
	this->end = other.end;
	this->meshPtr = other.meshPtr;
	return *this;
}

Proxy & Proxy::operator=(const Proxy & other)
{
	this->label		= other.label;
	this->seed		= other.seed;
	this->valid		= other.valid;
	this->normal	= other.normal;
	this->centroid	= other.centroid;
	this->borderHalfEdge	= other.borderHalfEdge;
	this->anchors			= other.anchors;
	this->borderEdgeCount	= other.borderEdgeCount;
	return *this;
}

HalfEdge Proxy::nextHalfEdgeOnBorder(MObject &mesh, const HalfEdge & he)
{
	if (false == isBorder(mesh, he)) return HalfEdge();
	auto nextHe = he.next();
	if (isBorder(mesh, nextHe)) return nextHe;
	return findHalfEdgeOnBorder(mesh, nextHe.vertex());
}

HalfEdge Proxy::findHalfEdgeOnBorder(MObject &mesh, VertexIndex v)
{
	MIntArray connectedIndices;
	MItMeshVertex vertexIter(mesh);
	int prev;
	vertexIter.setIndex(v, prev);
	vertexIter.getConnectedVertices(connectedIndices);
	// check each of outgoing halfedge from this vertex, see
	// if it is on border
	for (int i = 0; i < (int) connectedIndices.length(); i++)
	{
		HalfEdge he = HalfEdge(mesh, v, connectedIndices[i]);
		if (isBorder(mesh, he))
		{
			return he;
		}
	}
	// return invalid value if not found
	return HalfEdge();

}

bool Proxy::isBorder(MObject &mesh, const HalfEdge & he) const
{
	if (false == he.isValid())
	{
		return false;
	}
	if (true == he.isBoundary())
	{
		return false;
	}
	if (he.faceLabel() != this->label)
	{
		return false;
	}
	return he.twin().isBoundary() || he.twin().faceLabel() != this->label;
}

void Proxy::addAnchor(MObject &mesh, VertexIndex newAnchor)
{
	// find the border halfedge corresponding to the new anchor
	auto he = findHalfEdgeOnBorder(mesh, newAnchor);

	// TODO duplicate anchor check??

	// simple case
	if (anchors.size() < 2) {
		anchors.push_back(newAnchor);
		return;
	}

	// more complicated case:
	// travel along the border to find the anchor after it
	auto h = he;
	do {
		for (auto it = anchors.begin(); it != anchors.end(); it++) {
			if (h.vertex() == *it) {
				anchors.insert(it, newAnchor);
				return;
			}
		}
		h = nextHalfEdgeOnBorder(mesh, h);
	} while (h != he);
}
