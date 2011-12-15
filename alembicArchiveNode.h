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

Author:Ashley Retallack - ashley-r@blue-bolt.com

File:alembicArchiveNode.h

Dependency Graph Node: alembicArchiveNode

Based upon animaAlembicHolder by Olli Rajala @ anima

**/

#ifndef _alembicArchiveNode
#define _alembicArchiveNode

#include "Foundation.h"
#include "Transport.h"
#include "Scene.h"
#include "SceneManager.h"
#include "NodeIteratorVisitorHelper.h"

#include <Alembic/AbcGeom/IPolyMesh.h>
#include <Alembic/AbcGeom/ISubD.h>

#include <maya/MPxNode.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnData.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MBoundingBox.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MFloatVector.h>
#include <maya/MIntArray.h>
#include <maya/MFloatArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnStringData.h>
#include <maya/MTypeId.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagPath.h>
#include <maya/MNodeMessage.h>
#include <maya/MItSelectionList.h>

//include the renderman interface header for the procedural stuff
#include <ri.h>

#include <iostream>

#include "GlShaderHolder.h"

class alembicArchiveNode : public MPxLocatorNode
{
public:
                        alembicArchiveNode();
    virtual             ~alembicArchiveNode(); 

    virtual void postConstructor();
    virtual void copyInternalData( MPxNode* srcNode );
    virtual void draw( M3dView&, const MDagPath&, M3dView::DisplayStyle, M3dView::DisplayStatus);

    virtual bool isBounded() const;
    virtual MBoundingBox boundingBox()const ;
    virtual bool excludeAsLocator() const;

    virtual MStatus     compute( const MPlug& plug, MDataBlock& data );
    
    double setHolderTime(bool atClose) const;

    static  void*       creator();
    static  MStatus     initialize();

        MStatus     doSomething();
        
    std::string getSceneKey(bool proxy) const;

	MStatus emitCache(float relativeFrame=0.0f) ;

public:

    static  MObject     aAbcFile; 
    static  MObject     aObjectPath; 

    static  MObject     aShowProxy;
    static  MObject     aProxyPath;

    static  MObject     aTime;
    static  MObject     aTimeOffset;
    static  MObject     aShutterOpen;
    static  MObject     aShutterClose;
    static  MObject     aOutFps;
    static  MObject     aOutFrame;
    static  MObject     aBBMin;
    static  MObject     aBBMax;
    static	MObject		aBBSize;
    static	MObject		aBB;

    static	MObject		aOutUVs;

    static  MObject     aFurBBPad;
    static  MObject     aFurBBMin;
    static  MObject     aFurBBMax;
    static	MObject		aFurBBSize;
    static 	MObject		aFurBB;

    static  MObject     aBBCenter;

    static  MObject     aFurLOD;
    
    static  MObject     aShowBB;
    static  MObject     aFlipV;
//    static  MObject     aShowFurBB;
    static  MObject     aPolyAsSubD;
//    static  MObject     aExportFaceIds;
//    static  MObject     aFaceIdAttributeName;

    static  MTypeId     id;
    
    static SimpleAbcViewer::SceneState   abcSceneState;

    static SimpleAbcViewer::SceneManager abcSceneManager;

    std::string m_currscenekey;
    int m_bbmode;
    bool m_abcdirty;
    
    static GlShaderHolder glshader;
    
	MStatus archiveShadersAndAttribs() const;

	int archiveAttribs(	const MStringArray & shapeNames) const;

	int archiveShaders(	const MStringArray & shapeNames) const;

	MIntArray getUVShells() const;

private:

	void drawABox(const MVector &bbmin, const MVector &bbmax) ;

	WriterData oData;


};

inline MStatus bbValue(const MPlug &bbPlug, MVector &bmin, MVector &bmax) {
	MStatus st;

	double minx;
	double miny;
	double minz;
	double maxx;
	double maxy;
	double maxz;

//	unsigned nc = bbPlug.numChildren(&st);ert;
//	if (nc <2) return MS::kFailure;
	MPlug minPlug = bbPlug.child(0,&st);
	MPlug maxPlug = bbPlug.child(1,&st);

//	nc = minPlug.numChildren(&st);ert;
//	if (nc <3) return MS::kFailure;

	MPlug minXPlug = minPlug.child(0,&st);
	MPlug maxXPlug = minPlug.child(1,&st);
	MPlug minYPlug = minPlug.child(2,&st);

//	nc = maxPlug.numChildren(&st);ert;
//	if (nc <3) return MS::kFailure;
	MPlug maxYPlug = maxPlug.child(0,&st);
	MPlug minZPlug = maxPlug.child(1,&st);
	MPlug maxZPlug = maxPlug.child(2,&st);

	st =minXPlug.getValue (minx );
	st =maxXPlug.getValue (miny );
	st =minYPlug.getValue (minz );
	st =maxYPlug.getValue (maxx );
	st =minZPlug.getValue (maxy );
	st =maxZPlug.getValue (maxz );



	bmin=MVector(minx,miny,minz);
	bmax=MVector(maxx,maxy,maxz);
	return MS::kSuccess;
}

#endif
