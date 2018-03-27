//
// Copyright (C) Supernova Studio
//
// File: vsaTypes.h
// Header file for data types used in vsa
//
// MEL Command: isimp
//
// Author: George Zhu
//

#ifndef MAYA_VSA_TYPES_H
#define MAYA_VSA_TYPES_H

// C++ STL Includes
//
#include <set>
#include <vector>

// MAYA API Includes
//
#include <maya/MVector.h>
#include <maya/MPoint.h>

#define PriorityQueue std::multiset
#define GenericArray std::vector

typedef int ProxyLabel;
typedef int FaceIndex;
typedef int Size;
typedef int Index;

typedef MPoint Point3D;
typedef MVector Vector3D;


#endif //MAYA_VSA_TYPES_H
