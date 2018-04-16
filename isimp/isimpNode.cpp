//
// Copyright (C) Supernova Studio
// 
// File: isimpNode.cpp
//
// MEL Command: isimp
//
// Author: George Zhu
//

#include "isimpNode.h"

// Function Sets
//
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>

// General Includes
//
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MIOStream.h>

// Unique Node TypeId
MTypeId     isimpNode::id( 0x15206 );

// Node attributes
// (in addition to inMesh and outMesh defined by polyModifierNode)
//
MObject     isimpNode::cpList;
MObject     isimpNode::opType;
MObject		isimpNode::nProxies;
MObject		isimpNode::nIter;
MObject		isimpNode::eThres;
MObject		isimpNode::kHoles;


isimpNode::isimpNode()
{}

isimpNode::~isimpNode()
{}

MStatus isimpNode::compute(const MPlug& plug, MDataBlock& data)
//
//	Description:
//		This method computes the value of the given output plug based
//		on the values of the input attributes.
//
//	Arguments:
//		plug - the plug to compute
//		data - object that provides access to the attributes for this node
//
{
	MStatus status = MS::kSuccess;

	MDataHandle stateData = data.outputValue(state, &status);
	MCheckStatus(status, "ERROR getting state");

	// Check for the HasNoEffect/PassThrough flag on the node.
	//
	// (stateData is an enumeration standard in all depend nodes)
	// 
	// (0 = Normal)
	// (1 = HasNoEffect/PassThrough)
	// (2 = Blocking)
	// ...
	//
	if (stateData.asShort() == 1)
	{
		MDataHandle inputData = data.inputValue(inMesh, &status);
		MCheckStatus(status, "ERROR getting inMesh");

		MDataHandle outputData = data.outputValue(outMesh, &status);
		MCheckStatus(status, "ERROR getting outMesh");

		// Simply redirect the inMesh to the outMesh for the PassThrough effect
		//
		outputData.set(inputData.asMesh());
	}
	else
	{
		// Check which output attribute we have been asked to 
		// compute. If this node doesn't know how to compute it, 
		// we must return MS::kUnknownParameter
		// 
		if (plug == outMesh)
		{
			MDataHandle inputData = data.inputValue(inMesh, &status);
			MCheckStatus(status, "ERROR getting inMesh");

			MDataHandle outputData = data.outputValue(outMesh, &status);
			MCheckStatus(status, "ERROR getting outMesh");

			// Now, we get the value of the component list and the operation
			// type and use it to perform the mesh operation on this mesh
			//
			MDataHandle inputIDs = data.inputValue(cpList, &status);
			MCheckStatus(status, "ERROR getting componentList");

			MDataHandle opTypeData = data.inputValue(opType, &status);
			MCheckStatus(status, "ERROR getting opType");

			MDataHandle nProxData = data.inputValue(nProxies, &status);
			MCheckStatus(status, "ERROR getting nProxies");

			MDataHandle nIterData = data.inputValue(nIter, &status);
			MCheckStatus(status, "ERROR getting nIter");

			MDataHandle eThresData = data.inputValue(eThres, &status);
			MCheckStatus(status, "ERROR getting eThres");

			MDataHandle kHolesData = data.inputValue(kHoles, &status);
			MCheckStatus(status, "ERROR getting kHoles");

			// Copy the inMesh to the outMesh, so you can
			// perform operations directly on outMesh
			//
			outputData.set(inputData.asMesh());
			MObject mesh = outputData.asMesh();

			// Retrieve the ID list from the component list.
			//
			// Note, we use a component list to store the components
			// because it is more compact memory wise. (ie. comp[81:85]
			// is smaller than comp[81], comp[82],...,comp[85])
			//
			MObject compList = inputIDs.data();
			MFnComponentListData compListFn(compList);

			// Get what operation is requested and 
			// what type of component is expected for this operation.
			MeshOperation operationType = (MeshOperation)opTypeData.asShort();
			MFn::Type componentType =
				isimpFty::getExpectedComponentType(operationType);

			unsigned i;
			int j;
			MIntArray cpIds;

			for (i = 0; i < compListFn.length(); i++)
			{
				MObject comp = compListFn[i];
				if (comp.apiType() == componentType)
				{
					MFnSingleIndexedComponent siComp(comp);
					for (j = 0; j < siComp.elementCount(); j++)
						cpIds.append(siComp.element(j));
				}
			}

			Size numProxies = nProxData.asInt();
			Size numIterations = nIterData.asInt();
			double edgeSplitThreshold = eThresData.asDouble();
			bool keepHoles = (0 != kHolesData.asShort());

			// Set the mesh object and component List on the factory
			//
			fmeshOpFactory.setMesh(mesh);
			fmeshOpFactory.setComponentList(compList);
			fmeshOpFactory.setComponentIDs(cpIds);
			fmeshOpFactory.setMeshOperation(operationType);
			fmeshOpFactory.setVSAParams(numProxies, numIterations, edgeSplitThreshold, keepHoles);
			
			// Now, perform the meshOp
			//
			status = fmeshOpFactory.doIt();

			// Mark the output mesh as clean
			//
			outputData.setClean();
		}
		else
		{
			status = MS::kUnknownParameter;
		}
	}

	return status;
}

void* isimpNode::creator()
//
//	Description:
//		this method exists to give Maya a way to create new objects
//      of this type. 
//
//	Return Value:
//		a new object of this type
//
{
	return new isimpNode();
}

MStatus isimpNode::initialize()
//
//	Description:
//		This method is called to create and initialize all of the attributes
//      and attribute dependencies for this node type.  This is only called 
//		once when the node type is registered with Maya.
//
//	Return Values:
//		MS::kSuccess
//		MS::kFailure
//		
{
	MStatus				status;

	MFnTypedAttribute attrFn;
	MFnEnumAttribute enumFn;
	MFnNumericAttribute numIterFn;
	MFnNumericAttribute numProxyFn;
	MFnNumericAttribute eThresFn;
	MFnEnumAttribute kHolesFn;

	cpList = attrFn.create("inputComponents", "ics",
		MFnComponentListData::kComponentList);
	attrFn.setStorable(true);	// To be stored during file-save

	opType = enumFn.create("operationType", "oprt", 0, &status);
	enumFn.addField("init", 0);
	enumFn.addField("mesh", 1);
	enumFn.addField("add", 2);
	enumFn.addField("del", 3);
	enumFn.addField("paint", 4);
	enumFn.addField("color", 5);
	enumFn.setHidden(false);
	enumFn.setWritable(false);
	enumFn.setStorable(true);	// To be stored during file-save

	inMesh = attrFn.create("inMesh", "im", MFnMeshData::kMesh);
	attrFn.setStorable(true);	// To be stored during file-save

	// Attribute is read-only because it is an output attribute
	//
	outMesh = attrFn.create("outMesh", "om", MFnMeshData::kMesh);
	attrFn.setStorable(false);
	attrFn.setWritable(false);

	nProxies = numProxyFn.create("numProxies", "npx", MFnNumericData::kInt, 6);
	numProxyFn.setStorable(true);	// To be stored during file-save

	nIter = numIterFn.create("numIterations", "nit", MFnNumericData::kInt, 20);
	numIterFn.setStorable(true);	// To be stored during file-save

	eThres = eThresFn.create("egdeThreshold", "eth", MFnNumericData::kDouble, 1.0);
	eThresFn.setStorable(true);	// To be stored during file-save

	kHoles = kHolesFn.create("keepHoles", "khol", 0, &status);
	kHolesFn.addField("No", 0);
	kHolesFn.addField("Yes", 1);
	eThresFn.setStorable(true);	// To be stored during file-save


	// Add the attributes we have created to the node
	//
	status = addAttribute(cpList);
	if (!status)
	{
		status.perror("addAttribute");
		return status;
	}
	status = addAttribute(opType);
	if (!status)
	{
		status.perror("addAttribute");
		return status;
	}
	status = addAttribute(nProxies);
	if (!status)
	{
		status.perror("addAttribute");
		return status;
	}
	status = addAttribute(nIter);
	if (!status)
	{
		status.perror("addAttribute");
		return status;
	}
	status = addAttribute(eThres);
	if (!status)
	{
		status.perror("addAttribute");
		return status;
	}
	status = addAttribute(kHoles);
	if (!status)
	{
		status.perror("addAttribute");
		return status;
	}
	status = addAttribute(inMesh);
	if (!status)
	{
		status.perror("addAttribute");
		return status;
	}
	status = addAttribute(outMesh);
	if (!status)
	{
		status.perror("addAttribute");
		return status;
	}

	// Set up a dependency between the input and the output.  This will cause
	// the output to be marked dirty when the input changes.  The output will
	// then be recomputed the next time the value of the output is requested.
	//
	status = attributeAffects(inMesh, outMesh);
	if (!status)
	{
		status.perror("attributeAffects");
		return status;
	}

	status = attributeAffects(cpList, outMesh);
	if (!status)
	{
		status.perror("attributeAffects");
		return status;
	}

	status = attributeAffects(opType, outMesh);
	if (!status)
	{
		status.perror("attributeAffects");
		return status;
	}

	status = attributeAffects(nProxies, outMesh);
	if (!status)
	{
		status.perror("attributeAffects");
		return status;
	}

	status = attributeAffects(nIter, outMesh);
	if (!status)
	{
		status.perror("attributeAffects");
		return status;
	}

	status = attributeAffects(eThres, outMesh);
	if (!status)
	{
		status.perror("attributeAffects");
		return status;
	}

	status = attributeAffects(kHoles, outMesh);
	if (!status)
	{
		status.perror("attributeAffects");
		return status;
	}

	return MS::kSuccess;

}