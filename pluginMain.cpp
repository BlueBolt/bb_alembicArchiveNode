//
// Copyright (C) 2011 Anima Vitae Ltd. 
// 
// File: pluginMain.cpp
//
// Author: olli
//

#include "alembicArchiveNode.h"

#include <maya/MFnPlugin.h>

MStatus initializePlugin( MObject obj )
{ 
    MStatus   status;
    MFnPlugin plugin( obj, "BlueBolt Ltd.", "0.1", "Any");

    status = plugin.registerNode( "alembicArchiveNode", alembicArchiveNode::id, alembicArchiveNode::creator, alembicArchiveNode::initialize, MPxNode::kLocatorNode );
    if (!status) {
        status.perror("registerNode");
        return status;
    }

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
