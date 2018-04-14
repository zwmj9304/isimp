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
#include <maya/MTimer.h>
#include "vsaMesher.h"


HalfEdge::HalfEdge(MeshingContext &context, VertexIndex begin, VertexIndex end)
{
	this->begin		= begin;
	this->end		= end;
	this->ctx		= &context;
}

HalfEdge::HalfEdge(MeshingContext *ctx, VertexIndex begin, VertexIndex end)
{
	this->begin = begin;
	this->end = end;
	this->ctx = ctx;
}

HalfEdge HalfEdge::fromFace(MeshingContext &context, FaceIndex faceIdx)
{
	MStatus status;

	MIntArray vertexList;
	status = context.meshFn.getPolygonVertices(faceIdx, vertexList);
	if (status != MS::kSuccess || vertexList.length() < 3)
	{
		return HalfEdge();
	}
	return HalfEdge(context, vertexList[0], vertexList[1]);
}

HalfEdge HalfEdge::fromEdge(MeshingContext &context, EdgeIndex edgeIdx,
	VertexIndex startIdx)
{
	MStatus status;
	int2 vertexList;

	status = context.meshFn.getEdgeVertices(edgeIdx, vertexList);
	if (status != MS::kSuccess)
	{
		return HalfEdge();
	}
	if (startIdx < 0 || vertexList[0] == startIdx)
	{
		return HalfEdge(context, vertexList[0], vertexList[1]);
	}
	else
	{
		return HalfEdge(context, vertexList[1], vertexList[0]);
	}
}

HalfEdge HalfEdge::fromVertex(MeshingContext &context, VertexIndex vertIdx)
{
	MStatus status;

	MIntArray edgeList;
	int prevIdx;
	status = context.vertexIter.setIndex(vertIdx, prevIdx);
	status = context.vertexIter.getConnectedEdges(edgeList);

	if (status != MS::kSuccess || edgeList.length() < 1)
	{
		return HalfEdge();
	}
	return fromEdge(context, edgeList[0], vertIdx);
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

	MIntArray vertexList;
	status = ctx->meshFn.getPolygonVertices(faceIdx, vertexList);
	int degree = vertexList.length();
	for (VertexIndex v = 0; v < degree; v++)
	{
		if (vertexList[v] == end)
		{
			return HalfEdge(ctx, end, vertexList[(v + 1) % degree]);
		}
	}
	// Should not reach here
	//
	return HalfEdge();
}

HalfEdge HalfEdge::prev() const
{
	FaceIndex faceIdx = face();
	if (faceIdx < 0)
	{
		// TODO add support on border halfedges
		return HalfEdge();
	}

	MStatus status;

	MIntArray vertexList;
	status = ctx->meshFn.getPolygonVertices(faceIdx, vertexList);
	int degree = vertexList.length();
	for (VertexIndex v = 0; v < degree; v++)
	{
		if (vertexList[v] == begin)
		{
			return HalfEdge(ctx, vertexList[(v - 1 + degree) % degree], begin);
		}
	}
	// Should not reach here
	//
	return HalfEdge();

}

HalfEdge HalfEdge::twin() const
{
	return HalfEdge(ctx, end, begin);
}

int VSAMesher::m_counter[10];
double VSAMesher::m_time[10];

FaceIndex HalfEdge::face() const
{
	MTimer timer;
	MStatus status;
	MIntArray faceList;
	int prevIdx;

	timer.beginTimer();
	status = ctx->vertexIter.setIndex(begin, prevIdx);
	status = ctx->vertexIter.getConnectedFaces(faceList);
	timer.endTimer();  VSAMesher::m_counter[2]++; VSAMesher::m_time[2] += timer.elapsedTime();


	for (FaceIndex i = 0; i < (int) faceList.length(); i++)
	{
		timer.beginTimer();
		MIntArray faceVertices;
		status = ctx->meshFn.getPolygonVertices(faceList[i], faceVertices);
		int degree = faceVertices.length();
		timer.endTimer();  VSAMesher::m_counter[3]++; VSAMesher::m_time[3] += timer.elapsedTime();

		timer.beginTimer();
		for (VertexIndex v = 0; v < degree; v++)
		{
			if (faceVertices[v]					== begin &&
				faceVertices[(v + 1) % degree]	== end)
			{
				return faceList[i];
			}
		}
		timer.endTimer();  VSAMesher::m_counter[4]++; VSAMesher::m_time[4] += timer.elapsedTime();
	}
	// This means this halfedge is an border edge
	//
	return -1;
}

EdgeIndex HalfEdge::edge() const
{
	MStatus status;

	MIntArray edgeList;
	int prevIdx;
	status = ctx->vertexIter.setIndex(begin, prevIdx);
	status = ctx->vertexIter.getConnectedEdges(edgeList);
	for (EdgeIndex i = 0; i < (int) edgeList.length(); i++)
	{
		int2 edgeVertices;
		status = ctx->meshFn.getEdgeVertices(edgeList[i], edgeVertices);
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

ProxyLabel HalfEdge::faceLabel(Array<VSAFace> &faceList) const
{
	FaceIndex f = face();
	if (f < 0) return -1;
	return faceList[f].label;
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
	this->ctx = other.ctx;
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

HalfEdge Proxy::nextHalfEdgeOnBorder(MeshingContext &context, Array<VSAFace> &faceList, const HalfEdge & he)
{
	//if (false == isBorder(context, faceList, he)) return HalfEdge();
	//auto nextHe = he.next();
	//if (isBorder(context, faceList, nextHe)) return nextHe;
	//return findHalfEdgeOnBorder(context, faceList, nextHe.vertex());

	if (false == isBorder(context, faceList, he)) return HalfEdge();
	// Assuming counter-clockwise halfedge orientation
	// Triangle to the right side SHOULD belong to a different proxy
	//
	// Always go to the left side to find next halfedge on border
	auto nextHe = he.next();
	while (nextHe != he)
	{
		if (isBorder(context, faceList, nextHe))
		{
			return nextHe;
		}
		nextHe = nextHe.twin().next();
	}

	// Should not reach here
	return HalfEdge();
}

HalfEdge Proxy::findHalfEdgeOnBorder(MeshingContext &context, Array<VSAFace> &faceList, VertexIndex v)
{
	MIntArray connectedIndices;
	int prev;
	context.vertexIter.setIndex(v, prev);
	context.vertexIter.getConnectedVertices(connectedIndices);
	// check each of outgoing halfedge from this vertex, see
	// if it is on border
	for (int i = 0; i < (int) connectedIndices.length(); i++)
	{
		HalfEdge he = HalfEdge(context, v, connectedIndices[i]);
		if (isBorder(context, faceList, he))
		{
			return he;
		}
	}
	// return invalid value if not found
	return HalfEdge();

}

bool Proxy::isBorder(MeshingContext &context, Array<VSAFace> &faceList, const HalfEdge & he) const
{
	if (false == he.isValid())
	{
		return false;
	}
	if (true == he.isBoundary())
	{
		return false;
	}
	if (he.faceLabel(faceList) != this->label)
	{
		return false;
	}
	return he.twin().isBoundary() || he.twin().faceLabel(faceList) != this->label;
}

void Proxy::addAnchor(MeshingContext &context, Array<VSAFace> &faceList, VertexIndex newAnchor)
{
	// find the border halfedge corresponding to the new anchor
	auto he = findHalfEdgeOnBorder(context, faceList, newAnchor);

	// TODO duplicate anchor check??

	// simple case
	if (anchors.size() < 2) {
		anchors.push_back(newAnchor);
		return;
	}

	// more complicated case:
	// travel along the border to find the anchor after it
	// as in the hole case, no anchor will be added for this proxy
	auto h = he;
	do {
		for (auto it = anchors.begin(); it != anchors.end(); it++) {
			if (h.vertex() == *it) {
				anchors.insert(it, newAnchor);
				return;
			}
		}
		h = nextHalfEdgeOnBorder(context, faceList, h);
	} while (h != he);
}
