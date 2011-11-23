//
// Copyright (C) 2011 BlueBolt Ltd.
// 
// File: pluginMain.cpp
//
// Author: Ashley Retallack
//

#include "alembicArchiveNode.h"

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

MStatus initializePlugin( MObject obj )
{ 
    MStatus   status;
    const char * pluginVersion = "0.1.0";
    MFnPlugin plugin( obj, "BlueBolt Ltd.",pluginVersion, "Any");

    status = plugin.registerNode( "alembicArchiveNode", alembicArchiveNode::id, alembicArchiveNode::creator, alembicArchiveNode::initialize, MPxNode::kLocatorNode );
    if (!status) {
        status.perror("registerNode");
        return status;
    }

    MString info = "alembicArchiveNode v";
        info += pluginVersion;
        info += " using ";
        info += Alembic::AbcCoreAbstract::GetLibraryVersion().c_str();
        MGlobal::displayInfo(info);

    return status;
}

MStatus uninitializePlugin( MObject obj)
{
    MStatus   status;
    MFnPlugin plugin( obj );

    status = plugin.deregisterNode( alembicArchiveNode::id );
    if (!status) {
        status.perror("deregisterNode");
        return status;
    }

    return status;
}
