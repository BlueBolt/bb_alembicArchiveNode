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

File:bb_alembicArchiveShape.h

Dependency Graph Node: bb_alembicArchiveShape

Based upon animaAlembicHolder by Olli Rajala @ anima

**/

#ifndef _alembicArchiveNode
#define _alembicArchiveNode

#include "Foundation.h"
#include "Transport.h"
#include "Scene.h"
#include "SceneManager.h"
#include "NodeIteratorVisitorHelper.h"

//#include "AlembicGeometry.h"

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>

#include <maya/MPxNode.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MPxSurfaceShapeUI.h>

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnData.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnEnumAttribute.h>

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


#include <maya/MTextureEditorDrawInfo.h>

//include the renderman interface header for the procedural stuff
#include <ri.h>

#include <set>

#include <iostream>
#include <vector>

#include "GlShaderHolder.h"

// Geometry class
class AlembicArchiveGeom
{
public:
  AlembicArchiveGeom();
   MString objectPath;
   MString filename;
   MString geomLoaded;
   std::string scenekey;
   int mode;
   bool drawboundingBox;
   float frame;
   float frameOffset;
   bool useFrameExtension;
   bool useSubFrame;
   bool IsGeomLoaded;
   MBoundingBox bbox;
   bool deferStandinLoad;
   float scale;
   MPoint BBmin;
   MPoint BBmax;
   int dList;
   int updateView;
   int updateBBox;

   //std::vector<*AlembicMeshGeometry> m_geometryList;

};

// Shape class - defines the non-UI part of a shape node
class bb_alembicArchiveShape : public MPxSurfaceShape
{
public:
                        bb_alembicArchiveShape();
    virtual             ~bb_alembicArchiveShape();

    virtual void postConstructor();
    virtual MStatus     compute( const MPlug& plug, MDataBlock& data );
    virtual void copyInternalData( MPxNode* srcNode );
    //virtual void draw( M3dView&, const MDagPath&, M3dView::DisplayStyle, M3dView::DisplayStatus);

    virtual bool isBounded() const;
    virtual MBoundingBox boundingBox()const ;
    AlembicArchiveGeom* geometry();

    double setHolderTime(bool atClose) const;

    static  void*       creator();
    static  MStatus     initialize();

    std::string getSceneKey(bool proxy) const;

    MStatus emitCache(float relativeFrame=0.0f) ;

public:

    static  MObject    aAbcFile;
    static  MObject    aObjectPath;

    static  MObject    aShowProxy;
    static  MObject    aProxyPath;

    static  MObject    aTime;
    static  MObject    aTimeOffset;
    static  MObject    aShutterOpen;
    static  MObject    aShutterClose;
    static  MObject    aOutFps;
    static  MObject    aOutFrame;
    static  MObject    aBBMin;
    static  MObject    aBBMax;
    static  MObject    aBBSize;
    static  MObject    aBB;

    static  MObject    aOutUVs;
    static  MObject    aOutUDIMs;
    static  MObject    aObjects;

    // padded bounding box attributes
    static  MObject    aFurBBPad;
    static  MObject    aFurBBMin;
    static  MObject    aFurBBMax;
    static  MObject    aFurBBSize;
    static  MObject    aFurBB;

    static  MObject    aBBCenter;

    static  MObject    aFurLOD;
    
    static  MObject    aShowBB;
    static  MObject    aShowGL;
    static  MObject    aMode;
    static  MObject    aFlipV;
//    static  MObject     aShowFurBB;
    static  MObject    aPolyAsSubD;

    // Arnold stuff
    static  MObject    aSubDIterations;
    static  MObject    aSubDUVSmoothing;
    static  MObject    aMakeInstance;

    static  MObject    aExportFaceIds;

    std::vector<Alembic::Abc::IObject>  outIObjList;
    std::vector<Alembic::Abc::IObject>  fullOutIObjList;

    static  MTypeId     id; // nodeId
    static  MString     name; // nodeName
    
    static SimpleAbcViewer::SceneState   abcSceneState;

    static SimpleAbcViewer::SceneManager abcSceneManager;

    std::string m_currscenekey;
    MStringArray m_objects;
    MIntArray m_uvs;
    MIntArray m_udims;
    int m_bbmode;
    bool m_abcdirty;
    bool m_showbb;
    SimpleAbcViewer::ScenePtr m_scene;
    
    static GlShaderHolder glshader;
    
    MStatus archiveShadersAndAttribs() const;

    MStatus GetPointsFromAbc();

    int archiveAttribs(	const MStringArray & shapeNames) const;

    int archiveShaders(	const MStringArray & shapeNames) const;

    MStringArray getObjects();
    std::vector<MFloatArray> getUVs();
    MIntArray getUVShells();
    MIntArray getUDIMs();

private:
    AlembicArchiveGeom fGeometry;

    void walk(Alembic::Abc::IObject iObj);

    WriterData oData;

};


// UI class - defines the UI part of a shape node
class bb_alembicArchiveShapeUI: public MPxSurfaceShapeUI
{
public:
  bb_alembicArchiveShapeUI();
   virtual ~bb_alembicArchiveShapeUI();
   virtual void getDrawRequests(const MDrawInfo & info,
         bool objectAndActiveOnly, MDrawRequestQueue & requests);
   virtual void drawABox(const MVector &bbmin, const MVector &bbmax, bool poly) const;
   virtual void draw(const MDrawRequest & request, M3dView & view) const;
   virtual bool select(MSelectInfo &selectInfo, MSelectionList &selectionList,
         MPointArray &worldSpaceSelectPts) const;

   void getDrawRequestsWireFrame(MDrawRequest&, const MDrawInfo&);

   static void * creator();
   // Draw Tokens
   //
   enum
   {
     kDrawVertices, // component token
     kDrawWireframe,
     kDrawWireframeOnShaded,
     kDrawSmoothShaded,
     kDrawFlatShaded,
     kDrawBoundingBox,
     kDrawRedPointAtCenter,  // for userInteraction example code
     kLastToken
   };

}; // class bb_alembicArchiveShapeUI


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


	// Apply the vectors
	bmin=MVector(minx,miny,minz);
	bmax=MVector(maxx,maxy,maxz);
	return MS::kSuccess;
}
/*
inline bool abcfexists(const char *filename)
{
  ifstream ifile(filename);
  return ifile;
}
*/
#endif
