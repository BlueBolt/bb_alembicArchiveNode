#ifndef _alembicArchiveNode
#define _alembicArchiveNode
//
// Copyright (C) 2011 Anima Vitae Ltd. 
// 
// File: alembicArchiveNode.h
//
// Dependency Graph Node: alembicArchiveNode
//
// Author: olli
//

#include "Foundation.h"
#include "Transport.h"
#include "Scene.h"
#include "SceneManager.h"

#include <maya/MPxNode.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MFnNumericAttribute.h>
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
    static  MObject     aBBMinX;
    static  MObject     aBBMinY;
    static  MObject     aBBMinZ;
    static  MObject     aBBMaxX;
    static  MObject     aBBMaxY;
    static  MObject     aBBMaxZ;

    static  MObject     aFurBBPad;
    static  MObject     aFurBBMinX;
    static  MObject     aFurBBMinY;
    static  MObject     aFurBBMinZ;
    static  MObject     aFurBBMaxX;
    static  MObject     aFurBBMaxY;
    static  MObject     aFurBBMaxZ;

    static  MObject     aBBCenterX;
    static  MObject     aBBCenterY;
    static  MObject     aBBCenterZ;

    static  MObject     aFurLOD;
    
    static  MObject     aShowBB;

    static  MTypeId     id;
    
    static SimpleAbcViewer::SceneState   abcSceneState;

    static SimpleAbcViewer::SceneManager abcSceneManager;

    std::string m_currscenekey;
    int m_bbmode;
    bool m_abcdirty;
    
    static GlShaderHolder glshader;
    
private:


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
	MPlug minPlug = bbPlug.child(0,&st);er;
	MPlug maxPlug = bbPlug.child(1,&st);er;

//	nc = minPlug.numChildren(&st);ert;
//	if (nc <3) return MS::kFailure;

	MPlug minXPlug = minPlug.child(0,&st);er;
	MPlug maxXPlug = minPlug.child(1,&st);er;
	MPlug minYPlug = minPlug.child(2,&st);er;

//	nc = maxPlug.numChildren(&st);ert;
//	if (nc <3) return MS::kFailure;
	MPlug maxYPlug = maxPlug.child(0,&st);er;
	MPlug minZPlug = maxPlug.child(1,&st);er;
	MPlug maxZPlug = maxPlug.child(2,&st);er;

	st =minXPlug.getValue (minx );er;
	st =maxXPlug.getValue (miny );er;
	st =minYPlug.getValue (minz );er;
	st =maxYPlug.getValue (maxx );er;
	st =minZPlug.getValue (maxy );er;
	st =maxZPlug.getValue (maxz );er;



	bmin=MVector(minx,miny,minz);
	bmax=MVector(maxx,maxy,maxz);
	return MS::kSuccess;

#endif
