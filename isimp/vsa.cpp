//
// Copyright (C) Supernova Studio
//
// File: vsa.cpp
// Implementation of Variational Shape Approximation
// Methods in this file should be generic and portable to different platforms.
//
// Credit: Variational Shape Approximation
//	       David Cohen - Steiner, Pierre Alliez and Mathieu Desbrun, SIGGRAPH '2004.
// Author: George Zhu

#include "vsa.h"

void VSAFlooding::init(Array<VSAFace> &faceList, Array<Proxy> &proxyList, 
	Size numProxies)
{
	proxyList.clear();
	proxyList.resize(numProxies);
	Size numFaces	= (Size) faceList.size();
	Size offset		= numFaces / numProxies;
	ProxyLabel proxyLabel = 0;
	for (FaceIndex f = 0; f < numFaces && proxyLabel < numProxies; f += offset)
	{
		faceList[f].label = proxyLabel;
		proxyList[proxyLabel].label		= proxyLabel;
		proxyList[proxyLabel].seed		= f;
		proxyList[proxyLabel].centroid	= faceList[f].centroid;
		proxyList[proxyLabel].normal	= faceList[f].normal;
		proxyList[proxyLabel].valid		= true;
		proxyLabel++;
	}
}

void VSAFlooding::flood(Array<VSAFace> &faceList, Array<Proxy> &proxyList)
{
	// Global priority queue of faces
	auto metricComp = [](MetricFace mf1, MetricFace mf2) {
		return mf1.distance < mf2.distance; 
	};
	auto faceQueue = PriorityQueue<MetricFace, decltype(metricComp)> (metricComp);
	
	Size numProxies = (Size) proxyList.size();

	// For each proxy, first add its seed face to priority queue
	for (ProxyLabel label = 0; label < numProxies; label++)
	{
		if (false == proxyList[label].valid) continue;
		FaceIndex seedFace = proxyList[label].seed;
		faceQueue.insert(MetricFace(seedFace, label));
	}

	// For each face in the priority queue, perform flooding by adding its adjacent 
	// faces into the priority queue
	for (auto queueIter = faceQueue.begin(); queueIter != faceQueue.end(); )
	{
		FaceIndex index = queueIter->faceIndex;
		if (faceList[index].label == -1) // this means the face hasn't been labeled before
		{
			ProxyLabel possibleLabel = queueIter->possibleLabel;
			faceList[index].label = possibleLabel;
			// add all unlabeled adjacent faces to the priority queue
			for (int i = 0; i < 3; i++)
			{
				FaceIndex neighborIndex = faceList[index].neighbors[i];
				if (neighborIndex < 0) continue;
				auto &neighbor = faceList[neighborIndex];
				if (false == neighbor.isBoundary && neighbor.label == -1)
				{
					double distortionError = 
						calcDistortionError(neighbor, proxyList[possibleLabel]);
					faceQueue.insert( 
						MetricFace(neighborIndex, possibleLabel, distortionError) 
					);
				}
			}
		}

		// pop the top of priority queue
		faceQueue.erase(queueIter);
		queueIter = faceQueue.begin();
	}
}

double VSAFlooding::calcDistortionError(const VSAFace &face, const Proxy &proxy)
{
	return face.area * (face.normal - proxy.normal).length();
}

void VSAFlooding::fitProxy(Array<VSAFace> &faceList, Array<Proxy> &proxyList)
{
	Size numProxies = (Size) proxyList.size();
	Array<double> totalArea(numProxies);
	Array<double> totalDistortionError(numProxies);
	Array<double> lowestNormalDiff(numProxies);

	Array<Vector3D> areaWeightedNormal(numProxies);
	Array<Point3D> areaWeightedCentroid(numProxies);
	// add up related values per proxy
	for (auto &f : faceList)
	{
		ProxyLabel label = f.label;
		if (label < 0) continue;
		totalArea[label]			+= f.area;
		totalDistortionError[label] += calcDistortionError(f, proxyList[label]);
		areaWeightedNormal[label]	+= f.normal * f.area;
		areaWeightedCentroid[label] += f.centroid * f.area;
	}

	// update normal and seed of all proxies
	for (ProxyLabel label = 0; label < numProxies; label++) 
	{
		if (!proxyList[label].valid) continue;
		proxyList[label].normal = areaWeightedNormal[label] / totalArea[label];
		proxyList[label].centroid = areaWeightedCentroid[label] / totalArea[label];
		// initialize with double max value
		lowestNormalDiff[label] = 4.0;
	}

	// We pick the face with the lowest difference of normal as the new seed
	for (FaceIndex i = 0; i < faceList.size(); i++)
	{
		auto &f = faceList[i];
		ProxyLabel label = f.label;
		if (label < 0) continue;
		double normalDiff = (f.normal - proxyList[label].normal).length();
		if (normalDiff < lowestNormalDiff[label])
		{
			proxyList[label].seed = i;
			lowestNormalDiff[label] = normalDiff;
		}
		// clear face label
		f.label = -1;
	}
}

void VSAFlooding::clear(Array<VSAFace> &faceList)
{
	for (auto &f : faceList)
	{
		f.label = -1;
	}
}