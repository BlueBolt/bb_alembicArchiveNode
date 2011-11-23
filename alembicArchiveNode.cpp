//
// Copyright (C) 2011 BlueBolt Ltd.
// 
// File: alembicArchiveNode.h
//
// Dependency Graph Node: alembicArchiveNode
//
// Author: Ashley Retallack
//

#include <maya/MCommandResult.h>
#include <math.h>
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
MObject     alembicArchiveNode::aBBMin;
MObject     alembicArchiveNode::aBBMax;
MObject 	alembicArchiveNode::aBBSize;
MObject 	alembicArchiveNode::aBB;

MObject     alembicArchiveNode::aFurBBPad;
MObject     alembicArchiveNode::aFurBBMin;
MObject     alembicArchiveNode::aFurBBMax;
MObject 	alembicArchiveNode::aFurBBSize;
MObject 	alembicArchiveNode::aFurBB;

MObject     alembicArchiveNode::aBBCenter;
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
{
}

void nodePreRemovalCallback( MObject& obj, void* data)
{
    alembicArchiveNode::alembicArchiveNode *node = (alembicArchiveNode::alembicArchiveNode *)data;
    alembicArchiveNode::abcSceneManager.removeScene(node->getSceneKey());
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
        col = MColor(0.8,1.0,0.8);
    } else if (status == 0) {
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


    
    if(plug == aBBMin || plug == aBBMax || plug == aBB || plug == aBBSize)
    {
        MBoundingBox bb = boundingBox();

        MDataHandle tmpData = data.outputValue(aBBMin);
        tmpData.set3Double(bb.min().x,bb.min().y,bb.min().z);
        tmpData.setClean();
        tmpData = data.outputValue(aBBMax);
        tmpData.set3Double(bb.max().x,bb.max().y,bb.max().z);
        tmpData.setClean();

        cout << "just did something!! :D" << endl;

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

    if( plug == aShowBB )
    {

        MBoundingBox bb = boundingBox();

        MString info = "Viewing BoundingBox";
            info +=  bb.min().x;
            info +=  bb.min().y;
            info +=  bb.min().z;
            info +=  bb.max().x;
            info +=  bb.max().y;
            info +=  bb.max().z;

            MGlobal::displayInfo(info);


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
	MFnCompoundAttribute cAttr;
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
	stat = addAttribute(aBB);


    aFurBBPad = nAttr.create( "furBbPad", "fbbpad", MFnNumericData::kFloat);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
	stat = addAttribute(aFurBBPad);

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
	stat = addAttribute(aFurBB);

    aBBCenter = nAttr.create( "bbCenter", "bbc", MFnNumericData::k3Double);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
	stat = addAttribute(aBBCenter);


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

    attributeAffects( aObjectPath, aBBMin );
    attributeAffects( aObjectPath, aBBMax );
    attributeAffects( aObjectPath, aFurBBMin );
    attributeAffects( aObjectPath, aFurBBMax );;
    attributeAffects( aObjectPath, aBBCenter );
    attributeAffects( aObjectPath, aShowBB );


    attributeAffects( aTime, aOutFps );
    attributeAffects( aTime, aOutFrame );
    attributeAffects( aTime, aBBMin );
    attributeAffects( aTime, aBBMax );
    attributeAffects( aTime, aFurBBMin );
    attributeAffects( aTime, aFurBBMax );
    attributeAffects( aTime, aBBCenter );
    attributeAffects( aTime, aShowBB );

    attributeAffects( aTimeOffset, aOutFps );
    attributeAffects( aTimeOffset, aOutFrame );
    attributeAffects( aTimeOffset, aBBMin );
    attributeAffects( aTimeOffset, aBBMax );
    attributeAffects( aTimeOffset, aFurBBMin );
    attributeAffects( aTimeOffset, aFurBBMax );
    attributeAffects( aTimeOffset, aBBCenter );
    attributeAffects( aTimeOffset, aShowBB );
    
    attributeAffects( aShutterOpen, aOutFps );
    attributeAffects( aShutterOpen, aBBMin );
    attributeAffects( aShutterOpen, aBBMax );
    attributeAffects( aShutterOpen, aFurBBMin );
    attributeAffects( aShutterOpen, aFurBBMax );
    attributeAffects( aShutterOpen, aShowBB );

    attributeAffects( aShutterClose, aOutFps );
    attributeAffects( aShutterClose, aBBMin );
    attributeAffects( aShutterClose, aBBMax );
    attributeAffects( aShutterClose, aFurBBMin );
    attributeAffects( aShutterClose, aFurBBMax );
    attributeAffects( aShutterOpen, aShowBB );

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
	MSelectionList allShapesList;
	//archiveAttribs(thisMObject());
	//archiveShaders(thisMObject());

	return st;
}

