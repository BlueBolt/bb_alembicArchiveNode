//
// Copyright (C) 2011 Anima Vitae Ltd. 
// 
// File: alembicArchiveNode.cpp
//
// Dependency Graph Node: alembicArchiveNode
//
// Author: olli
//

#include <maya/MCommandResult.h>

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


#include "boost/foreach.hpp"

// The id is a 32bit value used to identify this type of node in the binary file format.
MTypeId     alembicArchiveNode::id( 0x00114955 ); // something like 0x0018c04e - check /anima/maya/src/animaNodeIds.txt

MObject     alembicArchiveNode::aAbcFile;
MObject     alembicArchiveNode::aObjectPath;
MObject     alembicArchiveNode::aTime;
MObject     alembicArchiveNode::aTimeOffset;
MObject     alembicArchiveNode::aShutterOpen;
MObject     alembicArchiveNode::aShutterClose;
MObject     alembicArchiveNode::aOutFps;
MObject     alembicArchiveNode::aOutFrame;
MObject     alembicArchiveNode::aBBMinX;
MObject     alembicArchiveNode::aBBMinY;
MObject     alembicArchiveNode::aBBMinZ;
MObject     alembicArchiveNode::aBBMaxX;
MObject     alembicArchiveNode::aBBMaxY;
MObject     alembicArchiveNode::aBBMaxZ;
MObject     alembicArchiveNode::aFurBBPad;
MObject     alembicArchiveNode::aFurBBMinX;
MObject     alembicArchiveNode::aFurBBMinY;
MObject     alembicArchiveNode::aFurBBMinZ;
MObject     alembicArchiveNode::aFurBBMaxX;
MObject     alembicArchiveNode::aFurBBMaxY;
MObject     alembicArchiveNode::aFurBBMaxZ;
MObject     alembicArchiveNode::aBBCenterX;
MObject     alembicArchiveNode::aBBCenterY;
MObject     alembicArchiveNode::aBBCenterZ;
MObject     alembicArchiveNode::aFurLOD;
MObject		alembicArchiveNode::aShowBB;


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
//        std::cout << "plugDirtied: " << fn.name() << " " << plug.name() << " = " << plug.asString() << std::endl;
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
//        std::string key = node->getSceneKey();

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
//    MFnDagNode fn( nodeMO );
    if (plug.attribute()==alembicArchiveNode::aAbcFile || plug.attribute()==alembicArchiveNode::aObjectPath) {
        MFnDagNode fn( node->thisMObject() );
//        std::cout << "plugDirtied: " << fn.name() << " " << plug.name() << " = " << plug.asString() << std::endl;
        node->m_abcdirty = true;
    }
}

void abcChangedCallback( MNodeMessage::AttributeMessage msg, MPlug & plug, MPlug & otherPlug, void* data)
{/*
    alembicArchiveNode::alembicArchiveNode *node = (alembicArchiveNode::alembicArchiveNode *)data;
//    std::cout << "attrChanged: " << node->name() << " - " << plug.name() << "(other: " << otherPlug.name() << " )" << std::endl;
    if ( ((msg & MNodeMessage::kAttributeSet) || (msg & MNodeMessage::kOtherPlugSet)) && (plug.attribute()==alembicArchiveNode::aAbcFile || plug.attribute()==alembicArchiveNode::aObjectPath)) {//plug.partialName()=="af") {
//        MString file;
//        plug.getValue(file);

        MFnDagNode fn( node->thisMObject() );
        MString file;
        MPlug fplug  = fn.findPlug( alembicArchiveNode::aAbcFile, true );
        fplug.getValue(file);
        // expand env variables
        file = file.expandFilePath();
        
//        std::cout << "assetChanged! " << file << std::endl;
        
//        MFnDagNode fn( node->thisMObject() );
        MString objectPath;
        MPlug opplug  = fn.findPlug( alembicArchiveNode::aObjectPath, true );
        opplug.getValue(objectPath);

//        MPlug opplug  = fn.findPlug( alembicArchiveNode::aObjectPath, true );
//        opplug.getValue(objectPath);

        MString mkey = file+"/"+objectPath;
        std::string key = mkey.asChar();//node->getSceneKey();

        if (node->m_currscenekey != key) {
            alembicArchiveNode::abcSceneManager.removeScene(node->m_currscenekey);
            node->m_currscenekey = "";
        }

//        if (file != "") {
            // TODO: check file existance!!
            alembicArchiveNode::abcSceneManager.addScene(file.asChar(),objectPath.asChar());

            node->m_currscenekey = key;
//        }
    }
*/}

void nodePreRemovalCallback( MObject& obj, void* data)
{
//    std::cout << "pre removal" << std::endl;
    alembicArchiveNode::alembicArchiveNode *node = (alembicArchiveNode::alembicArchiveNode *)data;
    alembicArchiveNode::abcSceneManager.removeScene(node->getSceneKey());
}

alembicArchiveNode::alembicArchiveNode() {
//    std::cout << "class creator" << std::endl;
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
        
    std::string sceneKey = getSceneKey();
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
    // update scene if needed
    if (m_abcdirty) updateAbc(this);

//    std::cout << "draw" << std::endl;
//    timer::timer TIMER;
//    TIMER.start();

    MColor col;
    if (status == 8) {
//        col = colorRGB( status );
        col = MColor(0.25,1.0,0.5);
    } else {
        col = MColor(0.7,0.5,0.5);
    }

    view.beginGL();

    // this makes a copy of the current openGL settings so that anything
    // we change will not affect anything else maya draws afterwards.
    glPushAttrib( GL_ALL_ATTRIB_BITS );

//    glShadeModel( GL_SMOOTH );
    setHolderTime();

    // init gl shaders - DISABLED FOR NOW - not even sure if we can do this here
//    glshader.init((char *)vshader, (char *)fshader);


    glEnable(GL_LIGHTING);

    glEnable(GL_COLOR_MATERIAL);

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
//    glUseProgram(glshader.getProgram());    

    glColor3f(col.r,col.g,col.b);

    // draw the "scene" from the alembic file
    std::string sceneKey = getSceneKey();
    if (abcSceneManager.hasKey(sceneKey))
        abcSceneManager.getScene(sceneKey)->draw(abcSceneState);
        
    glFlush();
//    glFinish();


    // restore the old openGL settings
    glPopAttrib();
    // think glPopMatrix()

    //add the boundingbox

	MObject thisNode = thisMObject();

    MPlug bbPlug(thisNode, aShowBB );
    bool doBB = bbPlug.asBool() ;

    if (doBB){
    	MPlug lodPlug( thisNode, aLodBB );
    	MVector bmin,bmax;
    	st = bbValue();
    	drawABox(lbmin, lbmax);

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

    std::string sceneKey = getSceneKey();
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




    // get attributes
/*    MDataHandle stringData = data.inputValue(aAbcFile, &returnStatus);
    if (MS::kSuccess != returnStatus) return returnStatus;
    MString stringAttr = stringData.asString();  

    MDataHandle booleanData = data.inputValue(aBooleanAttr, &returnStatus);
    if (MS::kSuccess != returnStatus) return returnStatus;
    bool booleanAttr = booleanData.asBool();  
*/
    MDataHandle floatData = data.inputValue(aFurBBPad, &returnStatus);
    if (MS::kSuccess != returnStatus) return returnStatus;
    float furBbPad = floatData.asFloat();  


    
    if(plug == aBBMinX || plug == aBBMinY || plug == aBBMinZ ||
        plug == aBBMaxX || plug == aBBMaxY || plug == aBBMaxZ )
    {
        MBoundingBox bb = boundingBox();

        MDataHandle tmpData = data.outputValue(aBBMinX);
        tmpData.setFloat(bb.min().x);
        tmpData.setClean();
        tmpData = data.outputValue(aBBMinY);
        tmpData.setFloat(bb.min().y);
        tmpData.setClean();
        tmpData = data.outputValue(aBBMinZ);
        tmpData.setFloat(bb.min().z);
        tmpData.setClean();
        tmpData = data.outputValue(aBBMaxX);
        tmpData.setFloat(bb.max().x);
        tmpData.setClean();
        tmpData = data.outputValue(aBBMaxY);
        tmpData.setFloat(bb.max().y);
        tmpData.setClean();
        tmpData = data.outputValue(aBBMaxZ);
        tmpData.setFloat(bb.max().z);
        tmpData.setClean();
    }

    if(plug == aFurBBMinX || plug == aFurBBMinY || plug == aFurBBMinZ ||
        plug == aFurBBMaxX || plug == aFurBBMaxY || plug == aFurBBMaxZ )
    {
        MBoundingBox bb = boundingBox();

        MDataHandle tmpData = data.outputValue(aFurBBMinX);
        tmpData.setFloat(bb.min().x-furBbPad);
        tmpData.setClean();
        tmpData = data.outputValue(aFurBBMinY);
        tmpData.setFloat(bb.min().y-furBbPad);
        tmpData.setClean();
        tmpData = data.outputValue(aFurBBMinZ);
        tmpData.setFloat(bb.min().z-furBbPad);
        tmpData.setClean();
        tmpData = data.outputValue(aFurBBMaxX);
        tmpData.setFloat(bb.max().x+furBbPad);
        tmpData.setClean();
        tmpData = data.outputValue(aFurBBMaxY);
        tmpData.setFloat(bb.max().y+furBbPad);
        tmpData.setClean();
        tmpData = data.outputValue(aFurBBMaxZ);
        tmpData.setFloat(bb.max().z+furBbPad);
        tmpData.setClean();
    }

    if(plug == aBBCenterX || plug == aBBCenterY || plug == aBBCenterZ )
    {
        m_bbmode = 1; // switch this on for a sec

        MBoundingBox bb = boundingBox();

        MDataHandle tmpData = data.outputValue(aBBCenterX);
        tmpData.setFloat((bb.min().x+bb.max().x)/2.0);
        tmpData.setClean();
        tmpData = data.outputValue(aBBCenterY);
        tmpData.setFloat((bb.min().y+bb.max().y)/2.0);
        tmpData.setClean();
        tmpData = data.outputValue(aBBCenterZ);
        tmpData.setFloat((bb.min().z+bb.max().z)/2.0);
        tmpData.setClean();

        m_bbmode = 0; // back to default
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
    m_currscenekey = getSceneKey();

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
    MStatus             stat;

    aAbcFile = tAttr.create( "abcFile", "af", MFnStringData::kString );
//    tAttr.setWritable(true);
//    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

    aObjectPath = tAttr.create( "objectPath", "op", MFnStringData::kString );
//    tAttr.setWritable(true);
//    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);

/*    aBooleanAttr = nAttr.create( "booleanAttr", "bo", MFnNumericData::kBoolean );
    nAttr.setReadable(true);
    nAttr.setKeyable(true);
    nAttr.setDefault(false);
*/
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



    aBBMinX = nAttr.create( "bbMinX", "bbminx", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aBBMinY = nAttr.create( "bbMinY", "bbminy", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aBBMinZ = nAttr.create( "bbMinZ", "bbminz", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aBBMaxX = nAttr.create( "bbMaxX", "bbmaxx", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aBBMaxY = nAttr.create( "bbMaxY", "bbmaxy", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aBBMaxZ = nAttr.create( "bbMaxZ", "bbmaxz", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);


    aFurBBPad = nAttr.create( "furBbPad", "fbbpad", MFnNumericData::kFloat);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);

    aFurBBMinX = nAttr.create( "furBbMinX", "fbbminx", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aFurBBMinY = nAttr.create( "furBbMinY", "fbbminy", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aFurBBMinZ = nAttr.create( "furBbMinZ", "fbbminz", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aFurBBMaxX = nAttr.create( "furBbMaxX", "fbbmaxx", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aFurBBMaxY = nAttr.create( "furBbMaxY", "fbbmaxy", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aFurBBMaxZ = nAttr.create( "furBbMaxZ", "fbbmaxz", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);

    aBBCenterX = nAttr.create( "bbCenterX", "bbcx", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aBBCenterY = nAttr.create( "bbCenterY", "bbcy", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    aBBCenterZ = nAttr.create( "bbCenterZ", "bbcz", MFnNumericData::kFloat);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);



    aFurLOD = nAttr.create( "furLOD", "flod", MFnNumericData::kFloat, 1.0);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);


    aShowBB = nAttr.create( "showBoundingBox", "sbb", MFnNumericData::kBoolean, 0, &stat);
	nAttr.setKeyable( true );
	stat = addAttribute(aShowBB);

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
    addAttribute( aBBMinX );
    addAttribute( aBBMinY );
    addAttribute( aBBMinZ );
    addAttribute( aBBMaxX );
    addAttribute( aBBMaxY );
    addAttribute( aBBMaxZ );

    addAttribute( aFurBBPad );
    addAttribute( aFurBBMinX );
    addAttribute( aFurBBMinY );
    addAttribute( aFurBBMinZ );
    addAttribute( aFurBBMaxX );
    addAttribute( aFurBBMaxY );
    addAttribute( aFurBBMaxZ );

    addAttribute( aBBCenterX );
    addAttribute( aBBCenterY );
    addAttribute( aBBCenterZ );

    addAttribute( aFurLOD );

    addAttribute( aShowBB );


    attributeAffects( aAbcFile, aBBMinX );
    attributeAffects( aAbcFile, aBBMinY );
    attributeAffects( aAbcFile, aBBMinZ );
    attributeAffects( aAbcFile, aBBMaxX );
    attributeAffects( aAbcFile, aBBMaxY );
    attributeAffects( aAbcFile, aBBMaxZ );
    attributeAffects( aAbcFile, aFurBBMinX );
    attributeAffects( aAbcFile, aFurBBMinY );
    attributeAffects( aAbcFile, aFurBBMinZ );
    attributeAffects( aAbcFile, aFurBBMaxX );
    attributeAffects( aAbcFile, aFurBBMaxY );
    attributeAffects( aAbcFile, aFurBBMaxZ );
    attributeAffects( aAbcFile, aBBCenterX );
    attributeAffects( aAbcFile, aBBCenterY );
    attributeAffects( aAbcFile, aBBCenterZ );

    attributeAffects( aObjectPath, aBBMinX );
    attributeAffects( aObjectPath, aBBMinY );
    attributeAffects( aObjectPath, aBBMinZ );
    attributeAffects( aObjectPath, aBBMaxX );
    attributeAffects( aObjectPath, aBBMaxY );
    attributeAffects( aObjectPath, aBBMaxZ );
    attributeAffects( aObjectPath, aFurBBMinX );
    attributeAffects( aObjectPath, aFurBBMinY );
    attributeAffects( aObjectPath, aFurBBMinZ );
    attributeAffects( aObjectPath, aFurBBMaxX );
    attributeAffects( aObjectPath, aFurBBMaxY );
    attributeAffects( aObjectPath, aFurBBMaxZ );
    attributeAffects( aObjectPath, aBBCenterX );
    attributeAffects( aObjectPath, aBBCenterY );
    attributeAffects( aObjectPath, aBBCenterZ );


    attributeAffects( aTime, aOutFps );
    attributeAffects( aTime, aOutFrame );
    attributeAffects( aTime, aBBMinX );
    attributeAffects( aTime, aBBMinY );
    attributeAffects( aTime, aBBMinZ );
    attributeAffects( aTime, aBBMaxX );
    attributeAffects( aTime, aBBMaxY );
    attributeAffects( aTime, aBBMaxZ );
    attributeAffects( aTime, aFurBBMinX );
    attributeAffects( aTime, aFurBBMinY );
    attributeAffects( aTime, aFurBBMinZ );
    attributeAffects( aTime, aFurBBMaxX );
    attributeAffects( aTime, aFurBBMaxY );
    attributeAffects( aTime, aFurBBMaxZ );
    attributeAffects( aTime, aBBCenterX );
    attributeAffects( aTime, aBBCenterY );
    attributeAffects( aTime, aBBCenterZ );

    attributeAffects( aTimeOffset, aOutFps );
    attributeAffects( aTimeOffset, aOutFrame );
    attributeAffects( aTimeOffset, aBBMinX );
    attributeAffects( aTimeOffset, aBBMinY );
    attributeAffects( aTimeOffset, aBBMinZ );
    attributeAffects( aTimeOffset, aBBMaxX );
    attributeAffects( aTimeOffset, aBBMaxY );
    attributeAffects( aTimeOffset, aBBMaxZ );
    attributeAffects( aTimeOffset, aFurBBMinX );
    attributeAffects( aTimeOffset, aFurBBMinY );
    attributeAffects( aTimeOffset, aFurBBMinZ );
    attributeAffects( aTimeOffset, aFurBBMaxX );
    attributeAffects( aTimeOffset, aFurBBMaxY );
    attributeAffects( aTimeOffset, aFurBBMaxZ );
    attributeAffects( aTimeOffset, aBBCenterX );
    attributeAffects( aTimeOffset, aBBCenterY );
    attributeAffects( aTimeOffset, aBBCenterZ );
    
    attributeAffects( aShutterOpen, aOutFps );
    attributeAffects( aShutterOpen, aBBMinX );
    attributeAffects( aShutterOpen, aBBMinY );
    attributeAffects( aShutterOpen, aBBMinZ );
    attributeAffects( aShutterOpen, aBBMaxX );
    attributeAffects( aShutterOpen, aBBMaxY );
    attributeAffects( aShutterOpen, aBBMaxZ );
    attributeAffects( aShutterOpen, aFurBBMinX );
    attributeAffects( aShutterOpen, aFurBBMinY );
    attributeAffects( aShutterOpen, aFurBBMinZ );
    attributeAffects( aShutterOpen, aFurBBMaxX );
    attributeAffects( aShutterOpen, aFurBBMaxY );
    attributeAffects( aShutterOpen, aFurBBMaxZ );

    attributeAffects( aShutterClose, aOutFps );
    attributeAffects( aShutterClose, aBBMinX );
    attributeAffects( aShutterClose, aBBMinY );
    attributeAffects( aShutterClose, aBBMinZ );
    attributeAffects( aShutterClose, aBBMaxX );
    attributeAffects( aShutterClose, aBBMaxY );
    attributeAffects( aShutterClose, aBBMaxZ );
    attributeAffects( aShutterClose, aFurBBMinX );
    attributeAffects( aShutterClose, aFurBBMinY );
    attributeAffects( aShutterClose, aFurBBMinZ );
    attributeAffects( aShutterClose, aFurBBMaxX );
    attributeAffects( aShutterClose, aFurBBMaxY );
    attributeAffects( aShutterClose, aFurBBMaxZ );

    attributeAffects( aFurBBPad, aFurBBMinX );
    attributeAffects( aFurBBPad, aFurBBMinY );
    attributeAffects( aFurBBPad, aFurBBMinZ );
    attributeAffects( aFurBBPad, aFurBBMaxX );
    attributeAffects( aFurBBPad, aFurBBMaxY );
    attributeAffects( aFurBBPad, aFurBBMaxZ );

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
    MStatus stat;

    cout << "just did something!! :D" << endl;

    return MS::kSuccess;
}

std::string alembicArchiveNode::getSceneKey() const
{
    MFnDagNode fn( thisMObject() );
    MString abcfile;
    MPlug plug  = fn.findPlug( aAbcFile );
    plug.getValue( abcfile );
    abcfile = abcfile.expandFilePath();
    MString objectPath;
    plug  = fn.findPlug( aObjectPath );
    plug.getValue( objectPath );

    return std::string((abcfile+"/"+objectPath).asChar());
}

int alembicArchiveNode::getDisplayBoundingBox() const
{
    MFnDagNode fn( thisMObject() );

    MPlug plug  = fn.findPlug( aShowBB );
    bool doBB;

    return doBB;
}

int alembicArchiveNode::archiveAttribs(	const MStringArray & shapeNames) const {
	MStatus st;
	unsigned ns = shapeNames.length();

//MStringArray allGeoAttribs;
	MSelectionList allGeoAttribs; // use a selectionList because it removes duplicates implicitly
	MStringArray geoAttribs;

	for (unsigned i=0;i<ns;i++) {
		geoAttribs.clear();

		MString cmd = "delightNodeQuery -shape \"";
		cmd += shapeNames[i];
		cmd += "\" -nodeType \"delightGeoAttribs\" -all";

		st = MGlobal::executeCommand(cmd,geoAttribs);

		unsigned ng = geoAttribs.length();

		for (unsigned j=0;j<ng;j++) {
			allGeoAttribs.add(geoAttribs[j]);
		}

	}
// now we have all the geoAttribs nodes in a selection list
	allGeoAttribs.getSelectionStrings(geoAttribs);
	unsigned ng = geoAttribs.length();
//cerr << "here: " << __LINE__ << endl;
	for (unsigned j=0;j<ng;j++) {
	// archive name must not have the namespace, so strip it out

		MString archiveName;
		MStringArray tmpSA;
		geoAttribs[j].split(':',tmpSA);
		archiveName = tmpSA[(tmpSA.length() -1)];


		RiArchiveBegin( const_cast <char *>(archiveName.asChar()) , RI_NULL);

		MString outputCmd = "DGA_output( \"";
		outputCmd += geoAttribs[j];
		outputCmd += "\",\"\")"; // second argument (renderpass for some legacy sohofx hook) is empty

		st = MGlobal::executeCommand(outputCmd);

		RiArchiveEnd();

	}
	return  ng;
}

int alembicArchiveNode::archiveShaders(	const MStringArray & shapeNames) const {
	MStatus st;
//cerr << "here: " << __LINE__ << endl;
	unsigned ns = shapeNames.length();
//MStringArray allGeoAttribs;
	unsigned ts;
//cerr << "here: " << __LINE__ << endl;
	MStringArray shaderTypes;
	shaderTypes.append("surface");
	shaderTypes.append("displacement");
	shaderTypes.append("atmosphere");
	shaderTypes.append("interior");
// cerr << "here: " << __LINE__ << endl;

	for (unsigned t=0;t<4;t++) {

		MSelectionList allShaders; // use a selectionList because it removes duplicates implicitly
		MStringArray allShaderStrings; // use a selectionList because it removes duplicates implicitly

		for (unsigned i=0;i<ns;i++) {
			MString shader;


			MString cmd = "delightNodeQuery -shape \"";
			cmd += shapeNames[i];
			cmd += "\" -nodeType \"";
			cmd += shaderTypes[t];
			cmd += "\" -highest";

			st = MGlobal::executeCommand(cmd,shader);

			if (shader != "") st = allShaders.add(shader);
		}

		allShaders.getSelectionStrings(allShaderStrings);
		unsigned nss = allShaderStrings.length();
		ts += nss;

		for (unsigned j=0;j<nss;j++) {

			MString shader = allShaderStrings[j];
		// archive name must not have the namespace, so strip it out
			MString archiveName;
			MStringArray tmpSA;
			shader.split(':',tmpSA);
			archiveName = tmpSA[(tmpSA.length() -1)];
			archiveName = (archiveName + " " +shaderTypes[t]);

			RiArchiveBegin( const_cast <char *>(archiveName.asChar()), RI_NULL);
			MString outputCmd = "DSN_output( \"";
			outputCmd += shader;
			outputCmd += "\",\"";
			outputCmd += shaderTypes[t];
			outputCmd += "\",\"\")";

			st = MGlobal::executeCommand(outputCmd);
			RiArchiveEnd();

		}
	}


// now we have all the shader nodes in a selection list

	return  ts;
}

MStatus alembicArchiveNode::archiveShadersAndAttribs() const {

// cerr << "in archiveShadersAndAttribs" << endl;
// we need to dig down to every shape contained within or under the sets or nodes connected to the lod geometry plug
	MStatus st = MS::kSuccess;
	MPlug geomPlug( thisMObject(), aLodGeometry );
	MSelectionList allShapesList;
	unsigned n = geomPlug.numElements();
//MStringArray lodNames;
	for (unsigned i = 0;i<n;i++){
		MPlug tmpPlug = geomPlug.elementByPhysicalIndex(i);
		MPlug setPlug =  tmpPlug.child(aLodGeometrySet,&st);
		allShapesList.merge(getShapesFromPlug(setPlug));
	}
	MStringArray shapeNames;
	allShapesList.getSelectionStrings(shapeNames);
	unsigned ns = shapeNames.length();

	archiveAttribs(shapeNames);
	archiveShaders(shapeNames);

	return st;
}

