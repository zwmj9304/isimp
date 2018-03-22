//
// Copyright (C) Supernova Studio
// 
// File: isimpMain.cpp
//
// MEL Command: isimp
//
// Author: George Zhu
//
////////////////////////////////////////////////////////////////////////
// DESCRIPTION:
// 
// Produces the command "meshOp" and the dependency node "isimpNode".
//
// Demonstrates the use of the high level polygon API methods that have been added to MFnMesh.
//
//	Syntax: meshOp $operationType 
//
//	Where, $operationType is one of the following: 
//
//		0 - Subdivide edge(s) 
// 
//		1 - Subdivide face(s)
// 
//		2 - Extrude edge(s) 
// 
//		3 - Extrude face(s) 
// 
//		4 - Collapse edge(s) 
// 
//		5 - Collapse face(s) 
//
//		6 - Duplicate face(s) 
//
//		7 - Extract face(s) 
//
//		8 - Split face(s)
//
//	Example usage:
//
//		- Select the appropriate component (edge, face)
//		- meshOp 2; 
//
//	Note that this plug-in re-uses the following files from "splitUVCmd".
//
//		- polyModifierNode.cpp 
//		- polyModifierCmd.cpp 
//		- polyModifierFty.cpp 
//
////////////////////////////////////////////////////////////////////////

#include "isimpCmd.h"
#include "isimpNode.h"

#include <maya/MFnPlugin.h>

MStatus initializePlugin(MObject obj)
//
//	Description:
//		this method is called when the plug-in is loaded into Maya.  It 
//		registers all of the services that this plug-in provides with 
//		Maya.
//
//	Arguments:
//		obj - a handle to the plug-in object (use MFnPlugin to access it)
//
{
	MStatus   status;
	MFnPlugin plugin(obj, "Supernova Studio", "1.0", "Any");

	status = plugin.registerCommand("isimp", isimp::creator);
	if (!status) {
		status.perror("registerCommand");
		return status;
	}

	status = plugin.registerNode("isimpNode",
		isimpNode::id,
		isimpNode::creator,
		isimpNode::initialize);
	if (!status) {
		status.perror("registerNode");
		return status;
	}

	return status;
}

MStatus uninitializePlugin(MObject obj)
//
//	Description:
//		this method is called when the plug-in is unloaded from Maya. It 
//		deregisters all of the services that it was providing.
//
//	Arguments:
//		obj - a handle to the plug-in object (use MFnPlugin to access it)
//
{
	MStatus   status;
	MFnPlugin plugin(obj);

	status = plugin.deregisterCommand("isimp");
	if (!status) {
		status.perror("deregisterCommand");
		return status;
	}

	status = plugin.deregisterNode(isimpNode::id);
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	return status;
}
