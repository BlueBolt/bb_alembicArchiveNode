//
// Copyright (C) 2011 BlueBolt Ltd.
// 
// File: pluginMain.cpp
//
// Author: Ashley Retallack
//

#include "alembicArchiveNode.h"
#include "delightCacheAlembic.h"

#include "errorMacros.h"

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

MStatus initializePlugin( MObject obj )
{ 
    MStatus   st;
    const char * pluginVersion = "0.1.0";
    MFnPlugin plugin( obj, "BlueBolt Ltd.",pluginVersion, "Any");

    st = plugin.registerNode( "alembicArchiveNode", alembicArchiveNode::id, alembicArchiveNode::creator, alembicArchiveNode::initialize, MPxNode::kLocatorNode );
    if (!st) {
        st.perror("registerNode");
        return st;
    }

	st = plugin.registerCommand( "delightCacheAlembic",delightCacheAlembic::creator ,delightCacheAlembic::newSyntax);


    MString info = "alembicArchiveNode v";
        info += pluginVersion;
        info += " using ";
        info += Alembic::AbcCoreAbstract::GetLibraryVersion().c_str();
        MGlobal::displayInfo(info);

    return st;
}

MStatus uninitializePlugin( MObject obj)
{
    MStatus   st;
    MFnPlugin plugin( obj );


    st = plugin.deregisterNode( alembicArchiveNode::id );
    if (!st) {
        st.perror("deregisterNode");
        return st;
    }

	st = plugin.deregisterCommand( "delightCacheAlembic" );er;

    return st;
}
