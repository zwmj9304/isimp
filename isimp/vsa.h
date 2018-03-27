//
// Copyright (C) Supernova Studio
//
// File: vsa.h
// Implementation of Variational Shape Approximation
// Methods in this file should be generic and portable to different platforms.
//
// Credit: Variational Shape Approximation
//	       David Cohen - Steiner, Pierre Alliez and Mathieu Desbrun, SIGGRAPH '2004.
// Author: George Zhu

#ifndef MAYA_VSA_H
#define MAYA_VSA_H

#include "vsaTypes.h"
#include "vsaProxy.h"
#include "vsaFace.h"

typedef struct MetricFace {
	FaceIndex faceIndex;		///< One face of mesh
	double distance;			///< Distance used for Lloyd algorithm
	ProxyLabel possibleLabel;	///< Possible label of cluster
	
	// Constructor
	MetricFace(FaceIndex faceIndex, ProxyLabel possibleLabel, double distance = 0.0) {
		this->faceIndex = faceIndex;
		this->possibleLabel = possibleLabel;
		this->distance = distance;
	}
} MetricFace;


class VSA {
public:
	static void init(GenericArray<VSAFace> &faceList, GenericArray<Proxy> &proxyList, Size numProxies);
	static void flood(GenericArray<VSAFace> &faceList, GenericArray<Proxy> &proxyList);
	static void fitProxy(GenericArray<VSAFace> &faceList, GenericArray<Proxy> &proxyList);
	static void clear(GenericArray<VSAFace> &faceList);

	static double calcDistortionError(const VSAFace &face, const Proxy &proxy);
};


#endif //MAYA_VSA_H
