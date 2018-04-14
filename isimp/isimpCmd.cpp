//
// Copyright (C) Supernova Studio
// 
// File: isimpCmd.cpp
//
// MEL Command: isimp
//
// Author: George Zhu
//

#include "isimpCmd.h"
#include "isimpNode.h"

// Function Sets
//
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSingleIndexedComponent.h>

// Iterators
//
#include <maya/MItSelectionList.h>
#include <maya/MItMeshPolygon.h>

// General Includes
//
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MArgList.h>

#include <maya/MIOStream.h>

isimp::isimp()
{
	fOperation = (MeshOperation)0;
}

isimp::~isimp() {}

void* isimp::creator()
{
	return new isimp();
}

bool isimp::isUndoable() const
{
	return true;
}

MStatus isimp::doIt(const MArgList& argList)
//
//	Description:
//		implements the MEL isimp command.
//
//	Arguments:
//		argList - the argument list that was passes to the command from MEL
//
//	Return Value:
//		MS::kSuccess - command succeeded
//		MS::kFailure - command failed (returning this value will cause the 
//                     MEL script that is being run to terminate unless the
//                     error is caught using a "catch" statement.
//
{
	MStatus status;
	if (true == checkInvalidInput(argList))
	{
		return MS::kFailure;
	}

	// Each mesh operation only supports one type of components
	// 
	MFn::Type componentType = isimpFty::getExpectedComponentType(fOperation);
	
	// Parse the selection list for selected components of the right type.
	// To simplify things, we only take the first object that we find with
	// selected components and operate on that object alone.
	//
	// All other objects are ignored and return warning messages indicating
	// this limitation.
	//
	MSelectionList selList;
	MGlobal::getActiveSelectionList(selList);
	MItSelectionList selListIter(selList);
	selListIter.setFilter(MFn::kMesh);

	// The isimp node only accepts a component list input, so we build
	// a component list using MFnComponentListData.
	//
	// MIntArrays could also be passed into the node to represent the ids,
	// but are less storage efficient than component lists, since consecutive 
	// components are bundled into a single entry in component lists.
	//
	MFnComponentListData compListFn;
	compListFn.create();
	bool found = false;
	bool foundMultiple = false;

	for (; !selListIter.isDone(); selListIter.next())
	{
		MDagPath dagPath;
		MObject component;
		selListIter.getDagPath(dagPath, component);

		// Check for selected components of the right type
		//
		if (component.apiType() == componentType)
		{
			if (!found)
			{
				// The variable 'component' holds all selected components 
				// on the selected object, thus only a single call to 
				// MFnComponentListData::add() is needed to store the selected
				// components for a given object.
				//
				compListFn.add(component);

				// Copy the component list created by MFnComponentListData
				// into our local component list MObject member.
				//
				fComponentList = compListFn.object();

				// Locally store the actual ids of the selected components so 
				// that this command can directly modify the mesh in the case 
				// when there is no history and history is turned off.
				//
				MFnSingleIndexedComponent compFn(component);

				// Ensure that this DAG path will point to the shape 
				// of our object. Set the DAG path for the polyModifierCmd.
				//
				dagPath.extendToShape();
				setMeshNode(dagPath);
				found = true;

				// Toggle Mesh Display Color, Need to operate on the node
				//
				MFnMesh(dagPath.node()).setDisplayColors(true);
			}
			else
			{
				// Break once we have found a multiple object holding 
				// selected components, since we are not interested in how 
				// many multiple objects there are, only the fact that there
				// are multiple objects.
				//
				foundMultiple = true;
				break;
			}
		}
	}

	if (foundMultiple)
	{
		displayWarning("Found more than one object with selected components.");
		displayWarning("Only operating on first found object.");
	}

	// Initialize the polyModifierCmd node type - mesh node already set
	//
	setModifierNodeType(isimpNode::id);

	if (found)
	{
		// Now, pass control over to the polyModifierCmd::doModifyPoly() method
		// to handle the operation.
		//
		status = doModifyPoly();

		if (status == MS::kSuccess)
		{
			setResult("isimp command succeeded!");
		}
		else
		{
			displayError("isimp command failed!");
		}
	}
	else
	{
		displayError(
			"isimp command failed: Unable to find selected components");
		status = MS::kFailure;
	}

	return status;
}

MStatus isimp::redoIt()
//
//	Description:
//		Implements redo for the MEL isimp command. 
//
//		This method is called when the user has undone a command of this type
//		and then redoes it.  No arguments are passed in as all of the necessary
//		information is cached by the doIt method.
//
//	Return Value:
//		MS::kSuccess - command succeeded
//		MS::kFailure - redoIt failed.  this is a serious problem that will
//                     likely cause the undo queue to be purged
//
{
	MStatus status;

	// Process the polyModifierCmd
	//
	status = redoModifyPoly();

	if (status == MS::kSuccess)
	{
		setResult("isimp command succeeded!");
	}
	else
	{
		displayError("isimp command failed!");
	}

	return status;
}

MStatus isimp::undoIt()
//
//	Description:
//		implements undo for the MEL meshOp command.  
//
//		This method is called to undo a previous command of this type.  The 
//		system should be returned to the exact state that it was it previous 
//		to this command being executed.  That includes the selection state.
//
//	Return Value:
//		MS::kSuccess - command succeeded
//		MS::kFailure - redoIt failed.  this is a serious problem that will
//                     likely cause the undo queue to be purged
//
{
	MStatus status;

	status = undoModifyPoly();

	if (status == MS::kSuccess)
	{
		setResult("meshOp undo succeeded!");
	}
	else
	{
		setResult("meshOp undo failed!");
	}

	return status;
}

MStatus isimp::initModifierNode(MObject modifierNode)
{
	MStatus status;

	// We need to tell the isimp node which components to operate on.
	// By overriding, the polyModifierCmd::initModifierNode() method,
	// we can insert our own modifierNode initialization code.
	//
	MFnDependencyNode depNodeFn(modifierNode);
	MObject cpListAttr = depNodeFn.attribute("inputComponents");

	// Pass the component list down to the meshOp node
	//
	MPlug cpListPlug(modifierNode, cpListAttr);
	status = cpListPlug.setValue(fComponentList);
	if (status != MS::kSuccess) return status;

	// Similarly for the operation type
	//
	MObject opTypeAttr = depNodeFn.attribute("operationType");
	MPlug opTypePlug(modifierNode, opTypeAttr);
	status = opTypePlug.setValue(fOperation);

	return status;
}

MStatus isimp::directModifier(MObject mesh)
{
	MStatus status;

	fmeshOpFactory.setMesh(mesh);
	fmeshOpFactory.setComponentList(fComponentList);
	fmeshOpFactory.setComponentIDs(fComponentIDs);
	fmeshOpFactory.setMeshOperation(fOperation);

	// Now, perform the meshOp
	//
	status = fmeshOpFactory.doIt();

	return status;
}

bool isimp::checkInvalidInput(const MArgList & argList)
{
	bool badArgument = false;
	// Only one parameter is expected to be passed to this command: the mesh
	// operation type. Get it, validate it or stop prematurely
	//
	if (argList.length() == 1)
	{
		int operationTypeArgument = argList.asInt(0);
		if (operationTypeArgument < 0
			|| operationTypeArgument > kMeshOperationCount - 1)
		{
			badArgument = true;
		}
		else
		{
			fOperation = (MeshOperation)operationTypeArgument;
		}
	}
	else badArgument = true;

	if (badArgument)
	{
		cerr << "Expecting one parameter: the operation type." << endl;
		cerr << "Valid types are: " << endl;
		cerr << "   0 - Flood." << endl;
		cerr << "   1 - Generate Simplified Mesh." << endl;
		cerr << "   2 - Add a Region." << endl;
		cerr << "   3 - Delete a Region." << endl;
		cerr << "   4 - Paint a New Region." << endl;
		cerr << "   5 - Turn on color display." << endl;
		displayError(" Expecting one parameter: the operation type.");
	}
	return badArgument;
}
