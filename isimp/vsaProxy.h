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

class Proxy {
public:

	Proxy() { this->valid = true; }

	/** The normal of the proxy */
	Vector3D	normal;
	/** The centroid of the proxy */
	Point3D		centroid;
	/**
	* The seed of a proxy is the face from which a region can be grown.
	* It is updated every iteration using Lloyd algorithm.
	*/
	FaceIndex	seed;
	/** Total area of this proxy */
	double		totalArea;
	/** Global approximation error over the proxy */
	double		totalDistorsion;
	/** The label of proxy */
	// ProxyLabel label;
	/** Whether this proxy is deleted */
	bool valid;

};


#endif //MAYA_VSA_PROXY_H