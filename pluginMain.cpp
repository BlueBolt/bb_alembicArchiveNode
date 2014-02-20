/**
(c)2011 Bluebolt Ltd.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
* Neither the name of BlueBolt nor the names of
its contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Author:Ashley Retallack - ashleyr-@blue-bolt.com

File:pluginMain.cpp

**/


#include "bb_alembicArchiveShape.h"
// #include "delightCacheAlembic.h"

#include "errorMacros.h"

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

MStatus initializePlugin( MObject obj )
{ 
    MStatus   st;
    const char * pluginVersion = "1.0.3";
    MFnPlugin plugin( obj, "BlueBolt Ltd.",pluginVersion, "Any");

    st = plugin.registerShape( bb_alembicArchiveShape::name, bb_alembicArchiveShape::id, bb_alembicArchiveShape::creator, bb_alembicArchiveShape::initialize, bb_alembicArchiveShapeUI::creator );

    if (!st) {
        st.perror("registerNode");
        return st;
    }

    // st = plugin.registerCommand( "delightCacheAlembic",delightCacheAlembic::creator ,delightCacheAlembic::newSyntax);

    MString info = bb_alembicArchiveShape::name;
        info += " v";
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


    st = plugin.deregisterNode( bb_alembicArchiveShape::id );
    if (!st) {
        st.perror("deregisterNode");
        return st;
    }

	// st = plugin.deregisterCommand( "delightCacheAlembic" );er;

    return st;
}
