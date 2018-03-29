#ifndef _isimpFty
#define _isimpFty

//
// Copyright (C) Supernova Studio
// 
// File: isimpFty.h
//
// MEL Command: isimp
//
// Author: George Zhu
//
// ***************************************************************************
//
// Overview:
//
//		The meshOp factory implements the actual meshOperation operation. 
//		It takes in three parameters:
//
//			1) A polygonal mesh
//			2) An array of component IDs
//          3) A mesh operation identifier
//
// Please refer to MFnMeshOperations to get more information on the different 
// mesh operations.
//

#include "polyModifierFty.h"

// General Includes
//
#include <maya/MObject.h>
#include <maya/MIntArray.h>
#include <maya/MString.h>

// Project Specific Includes
//
#include "vsa.h"

#define LABEL_BLIND_DATA_ID 15206
#define LABEL_BL_LONG_NAME "proxy_label"
#define LABEL_BL_SHORT_NAME "pxl"


enum MeshOperation
{
	kFlood = 0,
	kGenerate = 1,
	kRefresh = 2,
	kAddProxyBySeed = 3,
	kDeleteProxyBySeed = 4,
	kPaintProxyByFace = 5,

	// Number of valid operations
	kMeshOperationCount = 6
};

class isimpFty : public polyModifierFty
{

public:
	isimpFty();
	~isimpFty() override;

	// Modifiers
	//
	void		setMesh(MObject& mesh);
	void		setComponentList(MObject& componentList);
	void		setComponentIDs(MIntArray& componentIDs);
	void		setMeshOperation(MeshOperation operationType);
	void		setVSAParams(int numProxies, int numIterations);

	// Returns the type of component expected by a given mesh operation
	//
	static MFn::Type getExpectedComponentType(MeshOperation operationType);

	// polyModifierFty inherited methods
	//
	MStatus		doIt() override;

private:
	// Mesh Node - Note: We only make use of this MObject during a single 
	//					 call of the meshOperation plugin. It is never 
	//					 maintained and used between calls to the plugin as 
	//					 the MObject handle could be invalidated between 
	//                   calls to the plugin.
	//
	MObject		fMesh;

	// Selected Components and Operation to execute
	//
	MIntArray		fComponentIDs;
	MeshOperation	fOperationType;
	MObject			fComponentList;

	MStatus doFlooding();
	MStatus doReFlooding();
	MStatus doMeshing();
	MStatus doAddProxy();
	MStatus doDelProxy();
	MStatus doPaintProxy();

	GenericArray<VSAFace>	faceList;
	GenericArray<Proxy>		proxyList;

	// VSA Specific parameters
	//
	int	fNumProxies;			///< The number of proxies
	int	fNumIterations;			///< The maximum number of iterations
	double	edgeSplitThreshold;

	MStatus buildFaceNeighbors();
	MStatus getFloodingResult();
	MStatus checkOrCreateBlindDataType();
};

#endif