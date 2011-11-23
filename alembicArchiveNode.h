#ifndef _alembicArchiveNode
#define _alembicArchiveNode
//
// Copyright (C) 2011 BlueBolt Ltd.
// 
// File: alembicArchiveNode.h
//
// Dependency Graph Node: alembicArchiveNode
//
// Author: Ashley Retallack
//

#include "Foundation.h"
#include "Transport.h"
#include "Scene.h"
#include "SceneManager.h"

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
#include <maya/MIntArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnStringData.h>
#include <maya/MTypeId.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagPath.h>
#include <maya/MNodeMessage.h>
#include <maya/MItSelectionList.h>

//include the renderman interface header for the procedural stuff
#include "ri.h"

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
        
    std::string getSceneKey() const;

public:

    static  MObject     aAbcFile; 
    static  MObject     aObjectPath; 
//    static  MObject     aBooleanAttr; // example boolean attribute
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

    static  MObject     aFurBBPad;
    static  MObject     aFurBBMin;
    static  MObject     aFurBBMax;
    static	MObject		aFurBBSize;
    static 	MObject		aFurBB;

    static  MObject     aBBCenter;

    static  MObject     aFurLOD;
    
    static  MObject     aShowBB;
//    static  MObject     aShowFurBB;

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

private:

	void drawABox(const MVector &bbmin, const MVector &bbmax) ;


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
