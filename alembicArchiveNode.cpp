//******************************************************************************
// (c)2011 BlueBolt Ltd.  All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of BlueBolt nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Author:Ashley Retallack - ashley-r@blue-bolt.com
// 
// File:alembicArchiveNode.cpp
// 
// 
//******************************************************************************

// TODO: Add viewport 2.0 compatibility
// FIXME: Many dense archive nodes in scene crash maya when activating draw on
//	      startup

#include <maya/MCommandResult.h>
#include <math.h>

#include <bb_MayaIds.h>

#include "util.h"
#include "CreateSceneHelper.h"
#include "alembicArchiveNode.h"

#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MGlobal.h>
#include <maya/MTime.h>

#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <map>

#include "timer.h"
#include "errorMacros.h"

#include "boost/foreach.hpp"

// The id is a 32bit value used to identify this type of node in the binary file format.
MTypeId     alembicArchiveNode::id( k_alembicArchiveNode );

MObject     alembicArchiveNode::aAbcFile;
MObject     alembicArchiveNode::aObjectPath;

MObject     alembicArchiveNode::aShowProxy;
MObject     alembicArchiveNode::aProxyPath;

MObject     alembicArchiveNode::aTime;
MObject     alembicArchiveNode::aTimeOffset;
MObject     alembicArchiveNode::aShutterOpen;
MObject     alembicArchiveNode::aShutterClose;
MObject     alembicArchiveNode::aOutFps;
MObject     alembicArchiveNode::aOutFrame;
MObject     alembicArchiveNode::aBBMin;
MObject     alembicArchiveNode::aBBMax;
MObject 	alembicArchiveNode::aBBSize;
MObject 	alembicArchiveNode::aBB;

MObject 	alembicArchiveNode::aOutUVs;

MObject     alembicArchiveNode::aFurBBPad;
MObject     alembicArchiveNode::aFurBBMin;
MObject     alembicArchiveNode::aFurBBMax;
MObject 	alembicArchiveNode::aFurBBSize;
MObject 	alembicArchiveNode::aFurBB;

MObject     alembicArchiveNode::aBBCenter;
MObject     alembicArchiveNode::aFurLOD;
MObject		alembicArchiveNode::aShowBB;

MObject		alembicArchiveNode::aFlipV;
MObject		alembicArchiveNode::aPolyAsSubD;


SimpleAbcViewer::SceneState alembicArchiveNode::abcSceneState;
SimpleAbcViewer::SceneManager alembicArchiveNode::abcSceneManager;

GlShaderHolder alembicArchiveNode::glshader;

char vshader[] = { 
#include "glsl/abc.vert.xxd"
};
char fshader[] = {
#include "glsl/abc.frag.xxd"
};

void updateAbc(const void* data)
{
    alembicArchiveNode::alembicArchiveNode *node = (alembicArchiveNode::alembicArchiveNode *)data;

    MFnDagNode fn( node->thisMObject() );
    MString file;
    MPlug fplug  = fn.findPlug( alembicArchiveNode::aAbcFile );
    fplug.getValue(file);
    
    // expand env variables
    file = file.expandFilePath();
    
    MString objectPath;
    MPlug opplug  = fn.findPlug( alembicArchiveNode::aObjectPath );
    opplug.getValue(objectPath);

    MString mkey = file+"/"+objectPath;
    std::string key = mkey.asChar();//node->getSceneKey();

    if (node->m_currscenekey != key) {
        alembicArchiveNode::abcSceneManager.removeScene(node->m_currscenekey);
        node->m_currscenekey = "";
    }

    alembicArchiveNode::abcSceneManager.addScene(file.asChar(),objectPath.asChar());
    node->m_currscenekey = key;
    node->m_abcdirty = false;
}

void abcDirtiedCallback( MObject & nodeMO , MPlug & plug, void* data)
{
    alembicArchiveNode::alembicArchiveNode *node = (alembicArchiveNode::alembicArchiveNode *)data;
    if (plug.attribute()==alembicArchiveNode::aAbcFile || plug.attribute()==alembicArchiveNode::aObjectPath) {
        MFnDagNode fn( node->thisMObject() );
        node->m_abcdirty = true;
    }
}

void abcChangedCallback( MNodeMessage::AttributeMessage msg, MPlug & plug, MPlug & otherPlug, void* data)
{/*
    animaAlembicHolder::animaAlembicHolder *node = (animaAlembicHolder::animaAlembicHolder *)data;
//    std::cout << "attrChanged: " << node->name() << " - " << plug.name() << "(other: " << otherPlug.name() << " )" << std::endl;
    if ( ((msg & MNodeMessage::kAttributeSet) || (msg & MNodeMessage::kOtherPlugSet)) && (plug.attribute()==animaAlembicHolder::aAbcFile || plug.attribute()==animaAlembicHolder::aObjectPath)) {//plug.partialName()=="af") {
//        MString file;
//        plug.getValue(file);

        MFnDagNode fn( node->thisMObject() );
        MString file;
        MPlug fplug  = fn.findPlug( animaAlembicHolder::aAbcFile, true );
        fplug.getValue(file);
        // expand env variables
        file = file.expandFilePath();

//        std::cout << "assetChanged! " << file << std::endl;

//        MFnDagNode fn( node->thisMObject() );
        MString objectPath;
        MPlug opplug  = fn.findPlug( animaAlembicHolder::aObjectPath, true );
        opplug.getValue(objectPath);

//        MPlug opplug  = fn.findPlug( animaAlembicHolder::aObjectPath, true );
//        opplug.getValue(objectPath);

        MString mkey = file+"/"+objectPath;
        std::string key = mkey.asChar();//node->getSceneKey();

        if (node->m_currscenekey != key) {
            animaAlembicHolder::abcSceneManager.removeScene(node->m_currscenekey);
            node->m_currscenekey = "";
        }

//        if (file != "") {
            // TODO: check file existance!!
            animaAlembicHolder::abcSceneManager.addScene(file.asChar(),objectPath.asChar());

            node->m_currscenekey = key;
//        }
    }
*/
}

void nodePreRemovalCallback( MObject& obj, void* data)
{
    alembicArchiveNode::alembicArchiveNode *node = (alembicArchiveNode::alembicArchiveNode *)data;

    MFnDagNode fn( node->thisMObject() );

    MString objectPath;

	MPlug plug  = fn.findPlug( node->aObjectPath );
	plug.getValue( objectPath );

    alembicArchiveNode::abcSceneManager.removeScene(node->getSceneKey(false));
}

alembicArchiveNode::alembicArchiveNode() {
    m_bbmode = 0;
    m_currscenekey = "";
    m_abcdirty = false;
}

alembicArchiveNode::~alembicArchiveNode() {
}

double alembicArchiveNode::setHolderTime(bool atClose = false) const
{
    // update scene if needed
    if (m_abcdirty) updateAbc(this);
    
    MFnDagNode fn( thisMObject() );
    MTime time, timeOffset, shutterOpenT, shutterCloseT;
    float shutterOpen, shutterClose;
    MPlug plug  = fn.findPlug( aTime );
    plug.getValue( time );
    plug  = fn.findPlug( aTimeOffset );
    plug.getValue( timeOffset );
    plug  = fn.findPlug( aShutterOpen );
    plug.getValue( shutterOpen );
    plug  = fn.findPlug( aShutterClose );
    plug.getValue( shutterClose );
    shutterOpenT = MTime(shutterOpen,MTime::uiUnit());
    shutterCloseT = MTime(shutterClose,MTime::uiUnit());
    
    double dtime;
    if (!atClose) {
        dtime = time.as(MTime::kSeconds)+timeOffset.as(MTime::kSeconds)+shutterOpenT.as(MTime::kSeconds);
    } else {
        dtime = time.as(MTime::kSeconds)+timeOffset.as(MTime::kSeconds)+shutterCloseT.as(MTime::kSeconds);
    }


    std::string sceneKey = getSceneKey(false);
    if (abcSceneManager.hasKey(sceneKey))
        abcSceneManager.getScene(sceneKey)->setTime(dtime);

    return dtime;
}

void alembicArchiveNode::draw( M3dView& view,
		const MDagPath& DGpath,
		M3dView::DisplayStyle style,
		M3dView::DisplayStatus status )
{
	MStatus st;

	MObject thisNode = thisMObject();


    // update scene if needed
    if (m_abcdirty) updateAbc(this);

//    std::cout << "draw" << std::endl;
//    timer::timer TIMER;
//    TIMER.start();

    MColor col;

    if (status == 8) {
        col = MColor(0.8,1.0,0.8);
    } else if (status == 0) {
        col = MColor(0.25,1.0,0.5);
    } else {
    	col = MColor(0.7,0.5,0.5);
    }


    /*
    switch (status)  // From M3dView::DisplayStatus status  in the draw() command
    {
		case M3dView::kLead:
			col= MColor(0.26, 1.0, 0.64)  ;		// maya green
			bColOverride = true;
			break ;

		case M3dView::kActive:
			col = MColor(1.0, 1.0, 1.0)  ;		// maya white
			bColOverride = true;
			break ;

		case M3dView::kActiveAffected:
			col = MColor(0.78, 1.0, 0.78)  ;	// maya magenta
			bColOverride = true;
			break ;

		case M3dView::kTemplate:
			col = MColor(0.47, 0.47, 0.47)  ;	// maya template gray
			bColOverride = true;
			break ;

		case M3dView::kActiveTemplate:
			col = MColor(1.0, 0.47, 0.47)  ;	// maya selected template pink
			bColOverride = true;
			break ;

		default:
			//col = MColor(0.7,0.5,0.5);
			col = MColor(0.1, 0.2, 0.7) ;	// else set color as desired
    }
	*/

    view.beginGL();

    // this makes a copy of the current openGL settings so that anything
    // we change will not affect anything else maya draws afterwards.
    glPushAttrib( GL_ALL_ATTRIB_BITS );

    setHolderTime();

    // init gl shaders - DISABLED FOR NOW - not even sure if we can do this here
    // glshader.init((char *)vshader, (char *)fshader);

    if (style == M3dView::kWireFrame){

    	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    } else {
    	glShadeModel( GL_SMOOTH );
    	glEnable(GL_LIGHTING);
    	glEnable(GL_COLOR_MATERIAL);
    	glEnable (GL_BLEND);
    	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    // glUseProgram(glshader.getProgram());

    glColor3f(col.r,col.g,col.b);

    MFnDagNode fn( thisNode );

    bool doGL = 1;
    // draw the "scene" from the alembic file

    bool proxy = false;

    MPlug plug =  fn.findPlug( aShowProxy );
    plug.getValue( proxy );

    std::string sceneKey = getSceneKey(proxy);
    if (abcSceneManager.hasKey(sceneKey) && doGL)
        abcSceneManager.getScene(sceneKey)->draw(abcSceneState);
        
    glFlush();

    // restore the old openGL settings
    glPopAttrib();
    // think glPopMatrix()

    //add the boundingbox


    MPlug bbPlug(thisNode, aShowBB );
    bool doBB = bbPlug.asBool() ;

    if (doBB){

    	MBoundingBox bb = boundingBox();

    	//MPlug geoBBPlug( thisNode, aBB );
    	MVector bmin,bmax;
    	bmin=bb.min();
    	bmax=bb.max();

    	//st = bbValue(geoBBPlug, bmin,bmax);
    	drawABox(bmin, bmax);

    }

    view.endGL();
//    TIMER.stop();
//    TIMER.print();
}

bool alembicArchiveNode::isBounded() const
{
    return true;
}

MBoundingBox alembicArchiveNode::boundingBox() const
{
    // update scene if needed
    if (m_abcdirty) updateAbc(this);

    MBoundingBox bbox;

    std::string sceneKey = getSceneKey(false);
    if (abcSceneManager.hasKey(sceneKey)) {
        SimpleAbcViewer::Box3d bb;
        if (!m_bbmode) {
            setHolderTime(true); // first check bb at shutterClose
            bb = abcSceneManager.getScene(sceneKey)->getBounds();
            bbox.expand( MPoint( bb.min.x, bb.min.y, bb.min.z ) );
            bbox.expand( MPoint( bb.max.x, bb.max.y, bb.max.z ) );
        }
        setHolderTime(); // ...then at shutterOpen
        bb = abcSceneManager.getScene(sceneKey)->getBounds();
        bbox.expand( MPoint( bb.min.x, bb.min.y, bb.min.z ) );
        bbox.expand( MPoint( bb.max.x, bb.max.y, bb.max.z ) );
    }
    return bbox;
}

// don't treat locator as a normal maya locator
// locator won't be hidden if locators are set to not visible
bool alembicArchiveNode::excludeAsLocator() const
{
    return false;
}

MStatus alembicArchiveNode::compute( const MPlug& plug, MDataBlock& data )
{
    MStatus returnStatus;

    // update scene if needed
    if (m_abcdirty) updateAbc(this);

    MDataHandle floatData = data.inputValue(aFurBBPad, &returnStatus);
    if (MS::kSuccess != returnStatus) return returnStatus;
    float furBbPad = floatData.asFloat();  

    if (plug == aAbcFile || plug == aObjectPath ){


    	cout << "just did something!! :D" << endl;

    }
    
    if(plug == aBBMin || plug == aBBMax || plug == aBB || plug == aBBSize)
    {
        MBoundingBox bb = boundingBox();

        MDataHandle tmpData = data.outputValue(aBBMin);
        tmpData.set3Double(bb.min().x,bb.min().y,bb.min().z);
        tmpData.setClean();
        tmpData = data.outputValue(aBBMax);
        tmpData.set3Double(bb.max().x,bb.max().y,bb.max().z);
        tmpData.setClean();

    }

    if(plug == aFurBBMin || plug == aFurBBMax )
    {
        MBoundingBox bb = boundingBox();

        MDataHandle tmpData = data.outputValue(aFurBBMin);
        tmpData.set3Double(bb.min().x-furBbPad,
        		bb.min().y-furBbPad,
        		bb.min().z-furBbPad);
        tmpData.setClean();
        tmpData = data.outputValue(aFurBBMax);
        tmpData.set3Double(bb.max().x+furBbPad,
        		bb.max().y+furBbPad,
        		bb.max().z+furBbPad);
        tmpData.setClean();
    }

    if(plug == aBBCenter )
    {
        m_bbmode = 1; // switch this on for a sec

        MBoundingBox bb = boundingBox();

        MDataHandle tmpData = data.outputValue(aBBCenter);
        tmpData.set3Double((bb.min().x+bb.max().x)/2.0,
        		(bb.min().y+bb.max().y)/2.0,
        		(bb.min().z+bb.max().z)/2.0);
        tmpData.setClean();

        m_bbmode = 0; // back to default
    }

    // return the uvs as an array to outUVs attribute so multishell
    // textures will work
    if(plug == aOutUVs)
    {

        //Alembic::AbcGeom::IPolyMeshSchema schema = iNode.mMesh.getSchema();

        MDataHandle timeData = data.inputValue(aTime, &returnStatus);

        MTime time = timeData.asTime();

        MIntArray tmpUVArray = getUVShells();

        int nEle = tmpUVArray.length();

        int iVal;

        //plug.clear();

    	for (idx=0; idx < nEle; ++idx)
    	    {
				MPlug plugElement = plug.elementByLogicalIndex( idx, &stat) ;
				plugElement.setValue( iVal ) ;
    	    }


    }

    if(plug == aOutFps || plug == aOutFrame )
    {
        MDataHandle timeData = data.inputValue(aTime, &returnStatus);
        if (MS::kSuccess != returnStatus) return returnStatus;
        MTime time = timeData.asTime();  
        MDataHandle timeOffsetData = data.inputValue(aTimeOffset, &returnStatus);
        if (MS::kSuccess != returnStatus) return returnStatus;
        MTime timeOffset = timeOffsetData.asTime();  
        
        float fps = 25;
        if (MTime::uiUnit() == MTime::kFilm) {
            fps = 24;
        } else if (MTime::uiUnit() == MTime::kPALFrame) {
            fps = 25;
        } else if (MTime::uiUnit() == MTime::kNTSCFrame) {
            fps = 30;
        }

        MDataHandle tmpData = data.outputValue(aOutFps);
        tmpData.setFloat(fps);
        tmpData.setClean();
        tmpData = data.outputValue(aOutFrame);
        tmpData.setFloat((time+timeOffset).as(MTime::uiUnit()));
        tmpData.setClean();
    }

    return MS::kSuccess;
}


MIntArray alembicArchiveNode::getUVShells() const
{
	//get the current alembic file as a writer object

	double mTime = setHolderTime();

    std::string sceneKey = getSceneKey(false);

    CreateSceneVisitor visitor(mTime,false,
        MObject::kNullObj, CreateSceneVisitor::NONE, MString(sceneKey.c_str()));

	//retreive the list of uValues and vValues from the current level
	//in the alembic file

    WriterData mData;

	visitor.getData(mData);

	MFloatArray uValues;
	MFloatArray vValues;

	//list through the polymeshes

	unsigned int i;
	for (i=0; i < oData.mPolyMeshList.size();++i)
	{
		Alembic::AbcGeom::IPolyMeshSchema schema = oData.mPolyMeshList[i].mMesh.getSchema();

		Alembic::AbcGeom::IV2fGeomParam iUVs=schema.getUVsParam();

        // no interpolation for now
        Alembic::AbcCoreAbstract::index_t index, ceilIndex;
        getWeightAndIndex(mTime, iUVs.getTimeSampling(),
            iUVs.getNumSamples(), index, ceilIndex);

		Alembic::AbcGeom::IV2fGeomParam::Sample samp;
		iUVs.getIndexed(samp, Alembic::Abc::ISampleSelector(index));

		Alembic::AbcGeom::V2fArraySamplePtr uvPtr = samp.getVals();
		Alembic::Abc::UInt32ArraySamplePtr indexPtr = samp.getIndices();

		unsigned int numUVs = (unsigned int)uvPtr->size();
		uValues.setLength(uValues.length()+numUVs);
		vValues.setLength(vValues.length()+numUVs);
		for (unsigned int s = 0; s < numUVs; ++s)
		{
			uValues.append((*uvPtr)[s].x);
			vValues.append((*uvPtr)[s].y);
		}
	}


	//list through the subDSurfaces

	for (i=0; i < oData.mSubDList.size();++i)
	{
		Alembic::AbcGeom::ISubDSchema schema = oData.mSubDList[i].mMesh.getSchema();

		Alembic::AbcGeom::IV2fGeomParam iUVs=schema.getUVsParam();

        Alembic::AbcCoreAbstract::index_t index, ceilIndex;
        getWeightAndIndex(mTime, iUVs.getTimeSampling(),
            iUVs.getNumSamples(), index, ceilIndex);

		iUVs=schema.getUVsParam();

		Alembic::AbcGeom::IV2fGeomParam::Sample samp;
		iUVs.getIndexed(samp, Alembic::Abc::ISampleSelector(index));

		Alembic::AbcGeom::V2fArraySamplePtr uvPtr = samp.getVals();
		Alembic::Abc::UInt32ArraySamplePtr indexPtr = samp.getIndices();

		unsigned int numUVs = (unsigned int)uvPtr->size();
		uValues.setLength(uValues.length()+numUVs);
		vValues.setLength(vValues.length()+numUVs);
		for (unsigned int s = 0; s < numUVs; ++s)
		{
			uValues.append((*uvPtr)[s].x);
			vValues.append((*uvPtr)[s].y);
		}
	}


	//convert the two arrays in to a new arry of integers for the
	//shells that this file/path contains

	MIntArray uvShells;


    int minU=0;
    int maxU=1;
    int minV=0;
    int maxV=1;


    for (unsigned int u=0;u < uValues.length();++u){
    	if (u > maxU)
    		maxU=ceil(uValues[u]);
    }

    for (unsigned int v=0;v < vValues.length();++v){
    	if (v > maxV)
    		maxV=ceil(vValues[v]);
    }

    int t_minU = maxU;

    for (unsigned int mu=0;mu < uValues.length();++mu){
    	if (floor(uValues[mu]) < t_minU)
    		t_minU=floor(uValues[mu]);
    }


    minU=t_minU;

    int t_minV = maxV;

	for (unsigned int v=0;v < vValues.length();++v){
		if (floor(vValues[v]) < t_minV)
			t_minV=floor(vValues[v]);
	}

    minV=t_minV;

	for (unsigned int t=minU;t < maxU;++t){
		for (unsigned int u=0;u<uValues.length();++u){
    		if (u > t && u < t+1 && t+1){
    			bool matching = false;
    			for (int j = 0; (j < i) && (matching == false); j++)if (uvShells[i] == t+1) matching = true;
    			if (!matching) uvShells.append(t+1);

    		}
		}

	}

	return uvShells;


}  // setUVs

void* alembicArchiveNode::creator()
{
//    std::cout << "creator" << std::endl;
    return new alembicArchiveNode();
}

void alembicArchiveNode::postConstructor()
{
//    std::cout << "postconstruct" << std::endl;
    
    // callbacks
    MObject node = thisMObject();
    MNodeMessage::addAttributeChangedCallback( node, abcChangedCallback, this);
    MNodeMessage::addNodePreRemovalCallback( node, nodePreRemovalCallback, this); 
    MNodeMessage::addNodeDirtyPlugCallback( node, abcDirtiedCallback, this); 

    // call callback once on construct
    MFnDagNode fn( node );
    MPlug fplug  = fn.findPlug( alembicArchiveNode::aAbcFile, true );
    abcDirtiedCallback( node, fplug, this );

}

void alembicArchiveNode::copyInternalData( MPxNode* srcNode )
{
    // here we ensure that the scene manager stays up to date when duplicating nodes
    
    alembicArchiveNode::alembicArchiveNode *node = (alembicArchiveNode::alembicArchiveNode *)srcNode;

    MFnDagNode fn( node->thisMObject() );
    MString abcfile;
    MPlug plug  = fn.findPlug( aAbcFile );
    plug.getValue( abcfile );
    abcfile = abcfile.expandFilePath();

    MString objectPath;
    plug  = fn.findPlug( aObjectPath );
    plug.getValue( objectPath );

    abcSceneManager.addScene(abcfile.asChar(),objectPath.asChar());
    m_currscenekey = getSceneKey(false);

//    std::cout << "copyInternalData: " << abcfile << " " << objectPath << std::endl;

}


MStatus alembicArchiveNode::initialize()        
{
//    std::cout << "init" << std::endl;

    // init maya stuff

    MFnTypedAttribute tAttr;
    MFnNumericAttribute nAttr;
    MFnMessageAttribute mAttr;
    MFnUnitAttribute uAttr;
	MFnCompoundAttribute cAttr;
    MStatus             st;

    MFnStringData fileFnStringData;
    MObject fileNameDefaultObject = fileFnStringData.create("");

    aAbcFile = tAttr.create("abcFile", "af",
        MFnData::kString, fileNameDefaultObject);
    tAttr.setUsedAsFilename(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aObjectPath = tAttr.create( "objectPath", "op", MFnStringData::kString );
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aShowProxy = nAttr.create( "showProxy", "sp", MFnNumericData::kBoolean );
    nAttr.setReadable(true);
    nAttr.setKeyable(true);
    nAttr.setDefault(false);
	st = addAttribute(aShowProxy);

    aProxyPath = tAttr.create( "proxyPath", "pp", MFnStringData::kString );
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);
	st = addAttribute(aProxyPath);

    aTime = uAttr.create( "time", "t", MFnUnitAttribute::kTime );
    uAttr.setStorable(false);
    uAttr.setKeyable(true);
    uAttr.setReadable(true);
    uAttr.setWritable(true);

    aTimeOffset = uAttr.create( "timeOffset", "to", MFnUnitAttribute::kTime );
    uAttr.setStorable(true);
    uAttr.setKeyable(true);

    aShutterOpen = nAttr.create( "shutterOpen", "so", MFnNumericData::kFloat );
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    aShutterClose = nAttr.create( "shutterClose", "sc", MFnNumericData::kFloat );
    nAttr.setStorable(true);
    nAttr.setKeyable(true);

    aOutFps = nAttr.create( "outFps", "ofps", MFnNumericData::kFloat);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aOutFrame = nAttr.create( "outFrame", "of", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);

    //array of uvshells that this archive/path contains (starting with 1)
    aOutUVs = nAttr.create( "outUVs", "ouv", MFnNumericData::kInt);
    nAttr.setStorable(false);
    nAttr.setReadable(true);
	nAttr.setConnectable(true) ;
	nAttr.setArray(true);		// Set this to be an array!
	nAttr.setUsesArrayDataBuilder(true);		// Set this to be true also!



    aBBMin = nAttr.create( "geoBBMin", "gbbmin", MFnNumericData::k3Double);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aBBMax = nAttr.create( "geoBBMax", "gbbmax", MFnNumericData::k3Double);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aBBSize = nAttr.create( "geoBBSize", "gbbs", MFnNumericData::k3Double);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);


	aBB = cAttr.create("geoBoundingBox", "gbb");
	cAttr.addChild(aBBMin);
	cAttr.addChild(aBBMax);
	cAttr.addChild(aBBSize);
	cAttr.setStorable(true);
	cAttr.setHidden(false);
	st = addAttribute(aBB);


    aFurBBPad = nAttr.create( "furBbPad", "fbbpad", MFnNumericData::kFloat);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
	st = addAttribute(aFurBBPad);

    aFurBBMin = nAttr.create( "furBbMin", "fbbmin", MFnNumericData::k3Double);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aFurBBMax = nAttr.create( "furBbMax", "fbbmax", MFnNumericData::k3Double);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aFurBBSize = nAttr.create( "furBbSize", "fbbs", MFnNumericData::k3Double);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
	aFurBB = cAttr.create("furBoundingBox", "fbb");
	cAttr.addChild(aFurBBMin);
	cAttr.addChild(aFurBBMax);
	cAttr.addChild(aFurBBSize);
	cAttr.setStorable(true);
	cAttr.setHidden(false);
	st = addAttribute(aFurBB);

    aBBCenter = nAttr.create( "bbCenter", "bbc", MFnNumericData::k3Double);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
	st = addAttribute(aBBCenter);


    aFurLOD = nAttr.create( "furLOD", "flod", MFnNumericData::kFloat, 1.0);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);


    aShowBB = nAttr.create( "showBoundingBox", "sbb", MFnNumericData::kBoolean, 0, &st);
    nAttr.setHidden(false);
	nAttr.setKeyable( true );
	st = addAttribute(aShowBB);

    aFlipV = nAttr.create( "flipV", "fv", MFnNumericData::kBoolean, 1, &st);
    nAttr.setHidden(false);
	nAttr.setKeyable( true );
	st = addAttribute(aFlipV);

    aPolyAsSubD = nAttr.create( "polyAsSubD", "psd", MFnNumericData::kBoolean, 1, &st);
    nAttr.setHidden(false);
	nAttr.setKeyable( true );
	st = addAttribute(aPolyAsSubD);er;

/*    aMessageAttr = mAttr.create( "messageAttr", "me" );
    mAttr.setWritable(true);
    mAttr.setReadable(true);
    mAttr.setHidden(false);
    mAttr.setStorable(true);

    aOutStringAttr = tAttr.create( "outStringAttr", "os", MFnStringData::kString );
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
*/

    // Add the attributes we have created to the node
    //
    addAttribute( aAbcFile );
    addAttribute( aObjectPath );
//    addAttribute( aBooleanAttr );
    addAttribute( aTime );
    addAttribute( aTimeOffset );
    addAttribute( aShutterOpen );
    addAttribute( aShutterClose );
    addAttribute( aOutFps );
    addAttribute( aOutFrame );
//    addAttribute( aBBMin );
//    addAttribute( aBBMax );
//    addAttribute( aBBSize );

    addAttribute( aFurBBPad );
//    addAttribute( aFurBBMin );
//    addAttribute( aFurBBMax );
//    addAttribute( aFurBBSize );

    addAttribute( aBBCenter );

    addAttribute( aFurLOD );

    addAttribute( aShowBB );


    attributeAffects( aAbcFile, aBBMin );
    attributeAffects( aAbcFile, aBBMax );
    attributeAffects( aAbcFile, aBBSize );
    attributeAffects( aAbcFile, aBB );
    attributeAffects( aAbcFile, aShowBB );
    attributeAffects( aAbcFile, aFurBBMin );
    attributeAffects( aAbcFile, aFurBBMax );
    attributeAffects( aAbcFile, aFurBB );
    attributeAffects( aAbcFile, aBBCenter );
    attributeAffects( aAbcFile, aOutUVs );

    attributeAffects( aObjectPath, aBBMin );
    attributeAffects( aObjectPath, aBBMax );
    attributeAffects( aObjectPath, aFurBBMin );
    attributeAffects( aObjectPath, aFurBBMax );;
    attributeAffects( aObjectPath, aBBCenter );
    attributeAffects( aObjectPath, aShowBB );
    attributeAffects( aObjectPath, aOutUVs );


    attributeAffects( aTime, aOutFps );
    attributeAffects( aTime, aOutFrame );
    attributeAffects( aTime, aBBMin );
    attributeAffects( aTime, aBBMax );
    attributeAffects( aTime, aFurBBMin );
    attributeAffects( aTime, aFurBBMax );
    attributeAffects( aTime, aBBCenter );
    attributeAffects( aTime, aShowBB );
    attributeAffects( aTime, aOutUVs );

    attributeAffects( aTimeOffset, aOutFps );
    attributeAffects( aTimeOffset, aOutFrame );
    attributeAffects( aTimeOffset, aBBMin );
    attributeAffects( aTimeOffset, aBBMax );
    attributeAffects( aTimeOffset, aFurBBMin );
    attributeAffects( aTimeOffset, aFurBBMax );
    attributeAffects( aTimeOffset, aBBCenter );
    attributeAffects( aTimeOffset, aShowBB );
    attributeAffects( aTimeOffset, aOutUVs );
    
    attributeAffects( aShutterOpen, aOutFps );
    attributeAffects( aShutterOpen, aBBMin );
    attributeAffects( aShutterOpen, aBBMax );
    attributeAffects( aShutterOpen, aFurBBMin );
    attributeAffects( aShutterOpen, aFurBBMax );
    attributeAffects( aShutterOpen, aShowBB );
    attributeAffects( aShutterOpen, aOutUVs );

    attributeAffects( aShutterClose, aOutFps );
    attributeAffects( aShutterClose, aBBMin );
    attributeAffects( aShutterClose, aBBMax );
    attributeAffects( aShutterClose, aFurBBMin );
    attributeAffects( aShutterClose, aFurBBMax );
    attributeAffects( aShutterClose, aShowBB );
    attributeAffects( aShutterClose, aOutUVs );

    attributeAffects( aFurBBPad, aFurBBMin );
    attributeAffects( aFurBBPad, aFurBBMax );

    return MS::kSuccess;

}

//
// simple bounding box creator
void alembicArchiveNode::drawABox(const MVector &bmin, const MVector &bmax) {
	glBegin( GL_LINES );

	////////////////// BASE
	glVertex3f(float(bmin.x),float(bmin.y),float(bmin.z));
	glVertex3f(float(bmin.x),float(bmin.y),float(bmax.z));

	glVertex3f(float(bmin.x),float(bmin.y),float(bmax.z));
	glVertex3f(float(bmax.x),float(bmin.y),float(bmax.z));

	glVertex3f(float(bmax.x),float(bmin.y),float(bmax.z));
	glVertex3f(float(bmax.x),float(bmin.y),float(bmin.z));

	glVertex3f(float(bmax.x),float(bmin.y),float(bmin.z));
	glVertex3f(float(bmin.x),float(bmin.y),float(bmin.z));

	////////////////// TOP
	glVertex3f(float(bmin.x),float(bmax.y),float(bmin.z));
	glVertex3f(float(bmin.x),float(bmax.y),float(bmax.z));

	glVertex3f(float(bmin.x),float(bmax.y),float(bmax.z));
	glVertex3f(float(bmax.x),float(bmax.y),float(bmax.z));

	glVertex3f(float(bmax.x),float(bmax.y),float(bmax.z));
	glVertex3f(float(bmax.x),float(bmax.y),float(bmin.z));

	glVertex3f(float(bmax.x),float(bmax.y),float(bmin.z));
	glVertex3f(float(bmin.x),float(bmax.y),float(bmin.z));

	/////////////////// LEGS
	glVertex3f(float(bmin.x),float(bmin.y),float(bmin.z));
	glVertex3f(float(bmin.x),float(bmax.y),float(bmin.z));

	glVertex3f(float(bmin.x),float(bmin.y),float(bmax.z));
	glVertex3f(float(bmin.x),float(bmax.y),float(bmax.z));

	glVertex3f(float(bmax.x),float(bmin.y),float(bmax.z));
	glVertex3f(float(bmax.x),float(bmax.y),float(bmax.z));

	glVertex3f(float(bmax.x),float(bmin.y),float(bmin.z));
	glVertex3f(float(bmax.x),float(bmax.y),float(bmin.z));
	glEnd();

	// make the corners big points
	glPushAttrib(GL_CURRENT_BIT);
	glPointSize(5);
	glBegin( GL_POINTS );
	glVertex3f(float(bmin.x),float(bmin.y),float(bmin.z));
	glVertex3f(float(bmin.x),float(bmin.y),float(bmax.z));
	glVertex3f(float(bmin.x),float(bmax.y),float(bmin.z));
	glVertex3f(float(bmin.x),float(bmax.y),float(bmax.z));
	glVertex3f(float(bmax.x),float(bmin.y),float(bmin.z));
	glVertex3f(float(bmax.x),float(bmin.y),float(bmax.z));
	glVertex3f(float(bmax.x),float(bmax.y),float(bmin.z));
	glVertex3f(float(bmax.x),float(bmax.y),float(bmax.z));
	glEnd();
	glPopAttrib();

}

MStatus alembicArchiveNode::doSomething()
{
    MStatus st;

    cout << "just did something!! :D" << endl;

    return MS::kSuccess;
}

std::string alembicArchiveNode::getSceneKey(bool proxy) const
{
    MFnDagNode fn( thisMObject() );
    MString abcfile;
    MPlug plug  = fn.findPlug( aAbcFile );
    plug.getValue( abcfile );
    abcfile = abcfile.expandFilePath();

	MString objPath;
    //if not proxy show full path
    if (proxy){
		MPlug objPathPlug = fn.findPlug( aProxyPath );
		objPathPlug.getValue( objPath );
    } else {
    	MPlug objPathPlug = fn.findPlug(  aObjectPath );
    	objPathPlug.getValue( objPath );

    }
    return std::string((abcfile+"/"+objPath).asChar());
}

MStatus alembicArchiveNode::emitCache(float relativeFrame)  {

	//if (!hasCache()) return MS::kUnknownParameter;

	MStatus st;

	MObject thisNode = thisMObject();
	MString thisNodeName = MFnDependencyNode(thisNode).name();
////////////////get Object Path////////////////////

	MPlug objPathPlug( thisNode, aObjectPath );
	MString objPath;
	objPathPlug.getValue( objPath );

	////////////////////////////////////

	MPlug abcFilePlug( thisNode, aAbcFile );
	MString abcFile;
	abcFilePlug.getValue( abcFile );

	///////////// Get Flip UV ///////////////////////

	MPlug flipVPlug( thisNode, aFlipV );
	bool flipV;
	flipVPlug.getValue( flipV );

	///////////// Get polyAsSubD ///////////////////////

	MPlug subDPlug( thisNode, aPolyAsSubD );
	bool subD;
	subDPlug.getValue( subD );

	///////////// Get shutterOpen ///////////////////////

	MPlug shutterOpenPlug( thisNode, aShutterOpen );
	float shutterOpen;
	shutterOpenPlug.getValue( shutterOpen );

	///////////// Get shutterClose ///////////////////////

	MPlug shutterClosePlug( thisNode, aShutterClose );
	float shutterClose;
	shutterClosePlug.getValue( shutterClose );

////////////////get Bound////////////////////
//	MPlug BBPlug( thisNode, aBB );
//	MVector bmin,bmax;
//	st= bbValue(BBPlug, bmin, bmax) ;

	MBoundingBox bb = boundingBox();

	//MPlug geoBBPlug( thisNode, aBB );
	MVector bmin,bmax;
	bmin=bb.min();
	bmax=bb.max();

	MFloatVector bminf(bmin);
	MFloatVector bmaxf(bmax);
	RtBound bound = { bminf.x, bmaxf.x, bminf.y, bmaxf.y, bminf.z, bmaxf.z } ;
////////////////////////////////////

	RtString * progData = new RtString[2];

	MString argsString;
	if (objPath != ""){
		argsString += "-objectpath ";
		argsString += objPath;
	}
	if (flipV){
		argsString += " -flipv ";
	}
	if (subD){
		argsString += " -subd ";
	}
	if (shutterOpen != 0.0){
		argsString += " -shutteropen ";
		argsString += shutterOpen;
	}
	if (shutterClose != 0.0){
		argsString += " -shutterclose ";
		argsString += shutterClose;
	}
	argsString += " -filename ";
	argsString += abcFile;
	argsString += " -frame ";
	argsString += relativeFrame;

 	//cerr << "argsString :: " <<  argsString << endl;

	//st = createProgramStrings(geomPlug,progStrings, detailRanges, lodSetNames, blurSamples, relativeFrame) ;
	progData[0] = "AlembicRiProcedural";
	progData[1] = argsString.asChar();

	RiAttributeBegin();
	RiProcedural(progData,bound,RiProcDynamicLoad,RiProcFree);
	RiAttributeEnd();


	return MS::kSuccess;

}


