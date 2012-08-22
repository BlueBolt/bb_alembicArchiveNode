//******************************************************************************
// (c)2011-2012 BlueBolt Ltd.  All rights reserved.
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
// File:bb_alembicArchiveShape.cpp
// 
// 
//******************************************************************************

// TODO: Add viewport 2.0 compatibility
// TODO: Add SubD display
// TODO: Add out object tree attribute (output a list of objects in the archive)
// FIXME: Many dense archive nodes in scene crash maya when activating draw on
//	      startup

#include <maya/MCommandResult.h>
#include <math.h>
#include <sys/stat.h>

#include <bb_MayaIds.h>

#include "util.h"
#include "bb_alembicArchiveShape.h"

#include <maya/MPlug.h>
#include <maya/MPlugArray.h> // (?)
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MGlobal.h>
#include <maya/MTime.h>
#include <maya/MRenderUtil.h>


#include <maya/MBoundingBox.h>
#include <maya/MDrawRequest.h>
#include <maya/MDrawData.h>
#include <maya/MSelectionMask.h>
#include <maya/MSelectionList.h>

#include <maya/MHardwareRenderer.h>
#include <maya/MGLFunctionTable.h>

#include <sys/time.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <map>

#include "timer.h"
#include "errorMacros.h"

#include "boost/foreach.hpp"


// Object and component color defines
//
#define LEAD_COLOR              18      // green
#define ACTIVE_COLOR            15      // white
#define ACTIVE_AFFECTED_COLOR   8       // purple
#define DORMANT_COLOR           4       // blue
#define HILITE_COLOR            17      // pale blue
#define DORMANT_VERTEX_COLOR    8       // purple
#define ACTIVE_VERTEX_COLOR     16      // yellow

// The id is a 32bit value used to identify this type of node in the binary file format.
MTypeId    bb_alembicArchiveShape::id( k_bb_alembicArchiveShape );
MString    bb_alembicArchiveShape::name("bb_alembicArchiveShape");

MObject    bb_alembicArchiveShape::aAbcFile;
MObject    bb_alembicArchiveShape::aObjectPath;

MObject    bb_alembicArchiveShape::aShowProxy;
MObject    bb_alembicArchiveShape::aProxyPath;

MObject    bb_alembicArchiveShape::aTime;
MObject    bb_alembicArchiveShape::aTimeOffset;
MObject    bb_alembicArchiveShape::aShutterOpen;
MObject    bb_alembicArchiveShape::aShutterClose;
MObject    bb_alembicArchiveShape::aOutFps;
MObject    bb_alembicArchiveShape::aOutFrame;
MObject    bb_alembicArchiveShape::aBBMin;
MObject    bb_alembicArchiveShape::aBBMax;
MObject    bb_alembicArchiveShape::aBBSize;
MObject    bb_alembicArchiveShape::aBB;

MObject    bb_alembicArchiveShape::aOutUVs;
MObject    bb_alembicArchiveShape::aOutUDIMs;
MObject    bb_alembicArchiveShape::aObjects;

MObject    bb_alembicArchiveShape::aFurBBPad;
MObject    bb_alembicArchiveShape::aFurBBMin;
MObject    bb_alembicArchiveShape::aFurBBMax;
MObject    bb_alembicArchiveShape::aFurBBSize;
MObject    bb_alembicArchiveShape::aFurBB;

MObject    bb_alembicArchiveShape::aBBCenter;
MObject    bb_alembicArchiveShape::aFurLOD;

MObject    bb_alembicArchiveShape::aShowBB;
MObject    bb_alembicArchiveShape::aShowGL;

MObject    bb_alembicArchiveShape::aMode;

MObject    bb_alembicArchiveShape::aFlipV;
MObject    bb_alembicArchiveShape::aPolyAsSubD;

MObject    bb_alembicArchiveShape::aSubDIterations;
MObject    bb_alembicArchiveShape::aSubDUVSmoothing;
MObject    bb_alembicArchiveShape::aMakeInstance;

MObject    bb_alembicArchiveShape::aExportFaceIds;

SimpleAbcViewer::SceneState bb_alembicArchiveShape::abcSceneState;
SimpleAbcViewer::SceneManager bb_alembicArchiveShape::abcSceneManager;

GlShaderHolder bb_alembicArchiveShape::glshader;

char vshader[] = { 
#include "glsl/abc.vert.xxd"
};
char fshader[] = {
#include "glsl/abc.frag.xxd"
};

void updateAbc(const void* data)
{
    MStatus st;

    bb_alembicArchiveShape::bb_alembicArchiveShape *node = (bb_alembicArchiveShape::bb_alembicArchiveShape *)data;

    MFnDagNode fn( node->thisMObject() );
    MString fileName;
    MPlug fplug  = fn.findPlug( bb_alembicArchiveShape::aAbcFile );
    fplug.getValue(fileName);
    
    // expand env variables
    fileName = fileName.expandFilePath();
    
    MString objectPath;
    MPlug opplug  = fn.findPlug( bb_alembicArchiveShape::aObjectPath );
    opplug.getValue(objectPath);
    //catch if the file exists

    //struct stat buffer ;


    // no caching!
    Alembic::Abc::IArchive archive(Alembic::AbcCoreHDF5::ReadArchive(),
        fileName.asChar(), Alembic::Abc::ErrorHandler::Policy(),
        Alembic::AbcCoreAbstract::ReadArraySampleCachePtr());

    if (!archive.valid())
    {
        MString theError = "Cannot read file :: " + fileName;
        printError(theError);

    } else {

	//if ( abcfexists(file.asChar()) ){
		MString mkey = fileName+"/"+objectPath;
		std::string key = mkey.asChar();//node->getSceneKey();

		if (node->m_currscenekey != key) {
			bb_alembicArchiveShape::abcSceneManager.removeScene(node->m_currscenekey);
			node->m_currscenekey = "";
		}

		bb_alembicArchiveShape::abcSceneManager.addScene(fileName.asChar(),objectPath.asChar());
		node->m_scene=bb_alembicArchiveShape::abcSceneManager.getScene(key);
		node->m_currscenekey = key;
		node->m_abcdirty = false;
		node->m_uvs = node->getUVShells();
                node->m_udims = node->getUDIMs();
    //}
    }
    //node->m_objects = node->getObjects();

}



void abcDirtiedCallback( MObject & nodeMO , MPlug & plug, void* data)
{
    bb_alembicArchiveShape::bb_alembicArchiveShape *node = (bb_alembicArchiveShape::bb_alembicArchiveShape *)data;
    if (plug.attribute()==bb_alembicArchiveShape::aAbcFile || plug.attribute()==bb_alembicArchiveShape::aObjectPath) {
        MFnDagNode fn( node->thisMObject() );
        node->m_abcdirty = true;
    }
}

void abcChangedCallback( MNodeMessage::AttributeMessage msg, MPlug & plug, MPlug & otherPlug, void* data)
{
}

void nodePreRemovalCallback( MObject& obj, void* data)
{
    bb_alembicArchiveShape::bb_alembicArchiveShape *node = (bb_alembicArchiveShape::bb_alembicArchiveShape *)data;

    MFnDagNode fn( node->thisMObject() );

    MString objectPath;

	MPlug plug  = fn.findPlug( node->aObjectPath );
	plug.getValue( objectPath );

    bb_alembicArchiveShape::abcSceneManager.removeScene(node->getSceneKey(false));
}


AlembicArchiveGeom::AlembicArchiveGeom()
{
   filename  = "";
   objectPath = "";
   drawboundingBox = true;
   mode = 0;

   geomLoaded = "";
   scenekey = "";

   scale = 1.0f;
   BBmin = MPoint(-1.0f, -1.0f, -1.0f);
   BBmax = MPoint(1.0f, 1.0f, 1.0f);
   bbox = MBoundingBox(BBmin, BBmax);

   IsGeomLoaded = false;
   updateView = true;
   updateBBox = true;
   useSubFrame = false;
   useFrameExtension = false;
   dList = 0;
}


bb_alembicArchiveShape::bb_alembicArchiveShape() {
    m_bbmode = 0;
    m_currscenekey = "";
    m_abcdirty = false;
    m_objects = MStringArray();
    m_uvs = MIntArray();
    m_showbb = false;
}

bb_alembicArchiveShape::~bb_alembicArchiveShape() {
}

double bb_alembicArchiveShape::setHolderTime(bool atClose = false) const
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
    	m_scene->setTime(dtime);

    return dtime;
}


bool bb_alembicArchiveShape::isBounded() const
{
    return true;
}

//TODO: Make switch so if we need the shutter bounds or not (for the procedrual)

MBoundingBox bb_alembicArchiveShape::boundingBox() const
{
    // update scene if needed
    if (m_abcdirty) updateAbc(this);

    MBoundingBox bbox;

    std::string sceneKey = getSceneKey(false);
    if (abcSceneManager.hasKey(sceneKey)) {
        SimpleAbcViewer::Box3d bb;
        if (m_bbmode) {
            setHolderTime(true); // first check bb at shutterClose
            bb = abcSceneManager.getScene(sceneKey)->getBounds();
            bbox.expand( MPoint( bb.min.x, bb.min.y, bb.min.z ) );
            bbox.expand( MPoint( bb.max.x, bb.max.y, bb.max.z ) );
        }
        setHolderTime(); // ...then at shutterOpen
        bb = m_scene->getBounds();
        bbox.expand( MPoint( bb.min.x, bb.min.y, bb.min.z ) );
        bbox.expand( MPoint( bb.max.x, bb.max.y, bb.max.z ) );
    }
    return bbox;
}


MStatus bb_alembicArchiveShape::compute( const MPlug& plug, MDataBlock& data )
{
    MStatus returnStatus;

    // update scene if needed
    if (m_abcdirty) updateAbc(this);

    MDataHandle floatData = data.inputValue(aFurBBPad, &returnStatus);
    if (MS::kSuccess != returnStatus) return returnStatus;
    float furBbPad = floatData.asFloat();  

    if (plug == aAbcFile || plug == aObjectPath || plug == aSubDUVSmoothing ){

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
        m_bbmode = 0; // switch this on for a sec

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

    	data.setClean(plug);

        // ----- get uvshells and change the attribute to fit

        MIntArray tmpUVArray = m_uvs;

        unsigned int nEle = tmpUVArray.length();

        MArrayDataHandle outputArray = data.outputArrayValue(aOutUVs,
                                                             &returnStatus);
        MArrayDataBuilder builder(aOutUVs, nEle, &returnStatus);

    	for (unsigned int idx=0; idx < nEle; ++idx)
    	    {

    		MDataHandle outHandle = builder.addElement(idx);
    		outHandle.set(tmpUVArray[idx]);

    	    }

    	returnStatus = outputArray.set(builder);

    	returnStatus = outputArray.setAllClean();

    }

    if(plug == aOutUDIMs)
    {

        data.setClean(plug);

        // ----- get uvshells and change the attribute to fit

        MIntArray tmpUDIMArray = m_udims;

        unsigned int nEle = tmpUDIMArray.length();

        MArrayDataHandle outputArray = data.outputArrayValue(aOutUDIMs,
                                                             &returnStatus);
        MArrayDataBuilder builder(aOutUDIMs, nEle, &returnStatus);

        for (unsigned int idx=0; idx < nEle; ++idx)
            {

                MDataHandle outHandle = builder.addElement(idx);
                outHandle.set(tmpUDIMArray[idx]);

            }

        returnStatus = outputArray.set(builder);

        returnStatus = outputArray.setAllClean();

    }
    if(plug == aObjects)
    {

    	data.setClean(plug);

        // ----- get uvshells and change the attribute to fit

        MStringArray tmpObjArray = getObjects();

        unsigned int nEle = tmpObjArray.length();

        MArrayDataHandle outputArray = data.outputArrayValue(aObjects,
                                                             &returnStatus);
        MArrayDataBuilder builder(aObjects, nEle, &returnStatus);

    	for (unsigned int idx=0; idx < nEle; ++idx)
    	    {

    		MDataHandle outHandle = builder.addElement(idx);
    		outHandle.set(tmpObjArray[idx]);

    	    }

    	returnStatus = outputArray.set(builder);

    	returnStatus = outputArray.setAllClean();

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


void bb_alembicArchiveShape::walk(Alembic::Abc::IObject iObj)
{
	// Walk through the given object and return all of its children


    // Object has a name, a full name, some meta data,
    // and then it has a compound property full of properties.
    std::string path = iObj.getFullName();

    if ( path != "/" )
    {

		outIObjList.push_back(iObj);

    }

    // now the child objects
    for ( size_t i = 0 ; i < iObj.getNumChildren() ; i++ )
    {
        walk( Alembic::Abc::IObject( iObj, iObj.getChildHeader( i ).getName() ));

    }


}

MStringArray bb_alembicArchiveShape::getObjects()
{

	MStringArray objects;
    MStatus st = MS::kSuccess;

	//double mTime = setHolderTime();

    MFnDagNode fn( thisMObject() );
    MString abcfile;
    MPlug plug  = fn.findPlug( aAbcFile );
    plug.getValue( abcfile );

    std::string sceneKey = std::string((abcfile+"/").asChar());

    Alembic::Abc::IArchive archive = m_scene->getArchive();

    Alembic::Abc::IObject start = archive.getTop();

    /*
    Alembic::Abc::IObject start = abcSceneManager.getScene(key);

    if (abcSceneManager.hasKey(sceneKey)){

    	start =  m_scene->getTopObject();

    } else {
    	return objects;
    }
	*/

    size_t numChildren = start.getNumChildren();

    if (numChildren == 0) return objects;

    outIObjList.clear();

	walk(start);

	for (std::vector<Alembic::Abc::IObject>::iterator i = outIObjList.begin(); i != outIObjList.end(); i++)
	{
		//get the type for this object

		Alembic::Abc::IObject this_object = *i;

	 	//cout << "object :: " <<  this_object.getFullName() << endl;

		objects.append(MString(this_object.getFullName().c_str()));
	}



	return objects;

}

std::vector<MFloatArray> bb_alembicArchiveShape::getUVs()
{
	std::vector<MFloatArray> f_uvs[2];



}

MIntArray bb_alembicArchiveShape::getUVShells()
{
    MIntArray uvShells;
    MStatus st = MS::kSuccess;


    double mTime = setHolderTime();

    MFnDagNode fn( thisMObject() );
    MString abcfile;
    MPlug plug  = fn.findPlug( aAbcFile );
    plug.getValue( abcfile );

    std::string sceneKey = getSceneKey(false);

    Alembic::Abc::IObject start;


    if (abcSceneManager.hasKey(sceneKey)){

    	start =  m_scene->getTopObject();

    } else {
    	return uvShells;
    }

	start =  m_scene->getTopObject();

    MString objPath;
    plug  = fn.findPlug( aObjectPath );
    plug.getValue( objPath );

    size_t numChildren = start.getNumChildren();

    if (numChildren == 0) return uvShells;

    // list through the objects and retreve data based upon
    // the object being a polymesh or subD

    outIObjList.clear();

	walk(start);

	//retreive the list of uValues and vValues from the current level
	//in the alembic file

	MFloatArray uValues;
	MFloatArray vValues;

	//size_t noObjects = outIObjList.size() ;

	for (std::vector<Alembic::Abc::IObject>::iterator i = outIObjList.begin(); i != outIObjList.end(); i++)
	{
		//get the type for this object

		Alembic::Abc::IObject this_object = *i;

		MString ntype;

		if ( Alembic::AbcGeom::ISubD::matches(this_object.getHeader()) )
		{
			ntype="ISubD";

			Alembic::AbcGeom::ISubD mesh(this_object, Alembic::Abc::kWrapExisting);

			Alembic::AbcGeom::ISubDSchema schema = mesh.getSchema();

			Alembic::AbcGeom::IV2fGeomParam iUVs=schema.getUVsParam();

			if ( iUVs.valid() ){
				Alembic::AbcCoreAbstract::index_t index, ceilIndex;
				getWeightAndIndex(mTime, iUVs.getTimeSampling(),
					iUVs.getNumSamples(), index, ceilIndex);

				Alembic::AbcGeom::IV2fGeomParam::Sample samp;
				iUVs.getIndexed(samp, Alembic::Abc::ISampleSelector(index));

				Alembic::AbcGeom::V2fArraySamplePtr uvPtr = samp.getVals();
				Alembic::Abc::UInt32ArraySamplePtr indexPtr = samp.getIndices();

				unsigned int numUVs = (unsigned int)uvPtr->size();

				//uValues.setLength(numUVs);
				//vValues.setLength(numUVs);

				for (unsigned int s = 0; s < numUVs; s++)
				{
					uValues.append((*uvPtr)[s].x);
					vValues.append((*uvPtr)[s].y);
				}
			}


		}
		else if ( Alembic::AbcGeom::IPolyMesh::matches(this_object.getHeader()) )
		{
			ntype="IPolyMesh";

			Alembic::AbcGeom::IPolyMesh mesh(this_object, Alembic::Abc::kWrapExisting);

			Alembic::AbcGeom::IPolyMeshSchema schema = mesh.getSchema();

			Alembic::AbcGeom::IV2fGeomParam iUVs=schema.getUVsParam();

			if ( iUVs.valid() ){

				// no interpolation for now
				Alembic::AbcCoreAbstract::index_t index, ceilIndex;
				getWeightAndIndex(mTime, iUVs.getTimeSampling(),
					iUVs.getNumSamples(), index, ceilIndex);

				Alembic::AbcGeom::IV2fGeomParam::Sample samp;
				iUVs.getIndexed(samp, Alembic::Abc::ISampleSelector(index));

				Alembic::AbcGeom::V2fArraySamplePtr uvPtr = samp.getVals();
				Alembic::Abc::UInt32ArraySamplePtr indexPtr = samp.getIndices();

				unsigned int numUVs = (unsigned int)uvPtr->size();

				//uValues.setLength(numUVs);
				//vValues.setLength(numUVs);

				for (unsigned int s = 0; s < numUVs; s++)
				{
					uValues.append((*uvPtr)[s].x);
					vValues.append((*uvPtr)[s].y);
				}
			}

		}

	}


	//convert the two arrays in to a new arry of integers for the
	//shells that this file/path contains

	unsigned int minU=0;
    unsigned int maxU=1;
    unsigned int minV=0;
    unsigned int maxV=1;

    for (unsigned int u=0;u < uValues.length();++u){
    	if (ceil(uValues[u]) > maxU)
    		maxU=int(ceil(uValues[u]));
    }

    for (unsigned int v=0;v < vValues.length();++v){
    	if (ceil(vValues[v]) > maxV)
    		maxV=int(ceil(vValues[v]));
    }

    unsigned int t_minU = maxU;

    for (unsigned int mu=0;mu < uValues.length();++mu){
    	if (floor(uValues[mu]) < t_minU)
    		t_minU=int(floor(uValues[mu]));
    }


    minU=t_minU;

    unsigned int t_minV = maxV;

	for (unsigned int v=0;v < vValues.length();++v){
		if (floor(vValues[v]) < t_minV)
			t_minV=int(floor(vValues[v]));
	}

    minV=t_minV;

	for (int t=int(minU);t < int(maxU);t++){
		for (int u=0;u<int(uValues.length());u++){
    		if (uValues[u] > t && uValues[u] < t+1){
    			bool matching = false;
    			for (int j = 0; (j < int(uvShells.length())); j++)if (int(uvShells[j]) == t+1) matching = true;
    			if (!matching) uvShells.append(t+1);
    			//uvShells.append(t+1);
    		}
		}

	}

	//cout << "bb_alembicArchiveShape::getUVShells  uvShells > "<<  uvShells << endl;

	return uvShells;


}  // setUVs


MIntArray bb_alembicArchiveShape::getUDIMs()
{
    MIntArray UDIMs;
    MStatus st = MS::kSuccess;


    return UDIMs;
}

void* bb_alembicArchiveShape::creator()
{
//    std::cout << "creator" << std::endl;
    return new bb_alembicArchiveShape();
}

void bb_alembicArchiveShape::postConstructor()
{
//    std::cout << "postconstruct" << std::endl;
    
    // callbacks
    MObject node = thisMObject();
    MNodeMessage::addAttributeChangedCallback( node, abcChangedCallback, this);
    MNodeMessage::addNodePreRemovalCallback( node, nodePreRemovalCallback, this); 
    MNodeMessage::addNodeDirtyPlugCallback( node, abcDirtiedCallback, this); 

    // call callback once on construct
    MFnDagNode fn( node );
    MPlug fplug  = fn.findPlug( bb_alembicArchiveShape::aAbcFile, true );
    abcDirtiedCallback( node, fplug, this );

    setRenderable(true);

}

void bb_alembicArchiveShape::copyInternalData( MPxNode* srcNode )
{
    // here we ensure that the scene manager stays up to date when duplicating nodes
    
    bb_alembicArchiveShape::bb_alembicArchiveShape *node = (bb_alembicArchiveShape::bb_alembicArchiveShape *)srcNode;

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


MStatus bb_alembicArchiveShape::initialize()
{
//    std::cout << "init" << std::endl;

    // init maya stuff

    MFnTypedAttribute tAttr;
    MFnNumericAttribute nAttr;
    MFnMessageAttribute mAttr;
    MFnUnitAttribute uAttr;
    MFnCompoundAttribute cAttr;
    MFnEnumAttribute eAttr;
    MStatus             st;

    MFnStringData fileFnStringData;
    MObject fileNameDefaultObject = fileFnStringData.create("");

    aAbcFile = tAttr.create("abcFile", "af", MFnData::kString, fileNameDefaultObject);
    tAttr.setUsedAsFilename(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);
    st = addAttribute( aAbcFile );er;

    aObjectPath = tAttr.create( "objectPath", "opth", MFnStringData::kString );
    tAttr.setWritable(true);
    tAttr.setReadable(true);
    tAttr.setHidden(false);
    tAttr.setStorable(true);
    tAttr.setKeyable(true);
    st = addAttribute( aObjectPath );er;

    aShowProxy = nAttr.create( "showProxy", "sp", MFnNumericData::kBoolean );
    nAttr.setReadable(true);
    nAttr.setKeyable(true);
    nAttr.setDefault(false);
    st = addAttribute(aShowProxy);

    aProxyPath = tAttr.create( "proxyPath", "pp", MFnStringData::kString);
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

    aShutterOpen = nAttr.create( "shutterOpen", "so", MFnNumericData::kFloat , -0.25 );
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    aShutterClose = nAttr.create( "shutterClose", "sc", MFnNumericData::kFloat, 0.25 );
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
    aOutUVs = nAttr.create( "outUVs", "ouv", MFnNumericData::kInt,0,&st);
    nAttr.setStorable(false);
    nAttr.setReadable(true);
    nAttr.setWritable(true);
    nAttr.setArray(true);		// Set this to be an array!
    nAttr.setUsesArrayDataBuilder(true);		// Set this to be true also!
    nAttr.setHidden(true);
    st = addAttribute(aOutUVs);er;

    //array of uvshells that this archive/path contains (starting with 1)
    aOutUDIMs = nAttr.create( "outUDIMs", "udim", MFnNumericData::kInt,0,&st);
    nAttr.setStorable(false);
    nAttr.setReadable(true);
    nAttr.setWritable(true);
    nAttr.setArray(true);               // Set this to be an array!
    nAttr.setUsesArrayDataBuilder(true);                // Set this to be true also!
    nAttr.setHidden(true);
    st = addAttribute(aOutUDIMs);er;

    //array of objects that this archive/path contains
    aObjects = tAttr.create( "objects", "objs",  MFnStringData::kString,&st);
    tAttr.setStorable(false);
    tAttr.setReadable(true);
    tAttr.setWritable(true);
    tAttr.setArray(true);
    tAttr.setUsesArrayDataBuilder(true);
    tAttr.setHidden(true);
    st = addAttribute(aObjects);er;

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
    st = addAttribute(aBB);er;

    aFurBBPad = nAttr.create( "furBbPad", "fbbpad", MFnNumericData::kFloat);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    st = addAttribute(aFurBBPad);er;

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
    st = addAttribute(aFurBB);er;

    aBBCenter = nAttr.create( "bbCenter", "bbc", MFnNumericData::k3Double);
    nAttr.setStorable(false);
    nAttr.setWritable(true);
    nAttr.setReadable(true);
    nAttr.setHidden(true);
    st = addAttribute(aBBCenter);er;


    aFurLOD = nAttr.create( "furLOD", "flod", MFnNumericData::kFloat, 1.0);
    nAttr.setStorable(true);
    nAttr.setKeyable(true);


    aShowBB = nAttr.create( "showBoundingBox", "sbb", MFnNumericData::kBoolean, 0, &st);
    nAttr.setHidden(false);
    nAttr.setKeyable( true );
    st = addAttribute(aShowBB);er;

//    aShowGL = nAttr.create( "showGL", "sgl", MFnNumericData::kBoolean, 1, &st);
//    nAttr.setHidden(false);
//    nAttr.setKeyable( true );
//    st = addAttribute(aShowGL);er;

    aFlipV = nAttr.create( "flipV", "fv", MFnNumericData::kBoolean, 0, &st);
    nAttr.setHidden(false);
    nAttr.setKeyable( true );
    st = addAttribute(aFlipV);er;

    aPolyAsSubD = nAttr.create( "polyAsSubD", "psd", MFnNumericData::kBoolean, 1, &st);
    nAttr.setHidden(false);
    nAttr.setKeyable( true );
    st = addAttribute(aPolyAsSubD);er;

    // Arnold attributes

    aSubDIterations = nAttr.create( "ai_subDIterations", "si", MFnNumericData::kInt, 1 );
    nAttr.setStorable(true);
    nAttr.setKeyable(true);
    st = addAttribute(aSubDIterations);er;

    aSubDUVSmoothing = eAttr.create( "ai_subDUVSmoothing", "suv", 0 );  // can set default 0, 1, 2, etc..
    eAttr.setStorable(true);
    eAttr.setKeyable(true);
    eAttr.addField("pin_corners", 0) ;
    eAttr.addField("pin_borders", 1) ;
    eAttr.addField("linear", 2) ;
    eAttr.addField("smooth", 3) ;
    eAttr.setDefault(0) ;
    st = addAttribute(aSubDUVSmoothing);er;


    aMode = eAttr.create( "glMode", "gl", 1 );
    eAttr.setStorable(true);
    eAttr.setKeyable(true);
    eAttr.addField("Bounding Box", 0) ;
    eAttr.addField("Wireframe", 1) ;
    eAttr.addField("Smooth Shading", 2) ;
    eAttr.addField("Smooth with Wireframe", 3) ;
    eAttr.addField("Points", 4) ;
    eAttr.setDefault(1) ;
    st = addAttribute(aMode);er;

    aMakeInstance = nAttr.create( "makeInstance", "minst", MFnNumericData::kBoolean, 0, &st);
    nAttr.setHidden(false);
    nAttr.setKeyable( true );
    st = addAttribute(aMakeInstance);er;

    aExportFaceIds = nAttr.create( "exportFaceIds", "efid", MFnNumericData::kBoolean, 1, &st);
    nAttr.setHidden(false);
    nAttr.setKeyable( true );
    st = addAttribute(aExportFaceIds);er;

    // Add the attributes we have created to the node
    //
    addAttribute( aTime );er;
    addAttribute( aTimeOffset );er;
    addAttribute( aShutterOpen );er;
    addAttribute( aShutterClose );er;
    addAttribute( aOutFps );er;
    addAttribute( aOutFrame );er;
    addAttribute( aFurBBPad );er;
    addAttribute( aBBCenter );er;
    addAttribute( aFurLOD );er;
    addAttribute( aShowBB );er;

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
    attributeAffects( aAbcFile, aObjects );

    attributeAffects( aObjectPath, aBBMin );
    attributeAffects( aObjectPath, aBBMax );
    attributeAffects( aObjectPath, aFurBBMin );
    attributeAffects( aObjectPath, aFurBBMax );;
    attributeAffects( aObjectPath, aBBCenter );
    attributeAffects( aObjectPath, aShowBB );
    attributeAffects( aObjectPath, aOutUVs );
    //attributeAffects( aObjectPath, aObjects );


    attributeAffects( aTime, aOutFps );
    attributeAffects( aTime, aOutFrame );
    attributeAffects( aTime, aBBMin );
    attributeAffects( aTime, aBBMax );
    attributeAffects( aTime, aFurBBMin );
    attributeAffects( aTime, aFurBBMax );
    attributeAffects( aTime, aBBCenter );
    attributeAffects( aTime, aShowBB );
    attributeAffects( aTime, aOutUVs );
    attributeAffects( aTime, aObjects );

    attributeAffects( aTimeOffset, aOutFps );
    attributeAffects( aTimeOffset, aOutFrame );
    attributeAffects( aTimeOffset, aBBMin );
    attributeAffects( aTimeOffset, aBBMax );
    attributeAffects( aTimeOffset, aFurBBMin );
    attributeAffects( aTimeOffset, aFurBBMax );
    attributeAffects( aTimeOffset, aBBCenter );
    attributeAffects( aTimeOffset, aShowBB );
    attributeAffects( aTimeOffset, aOutUVs );
    attributeAffects( aTimeOffset, aObjects );
    
    attributeAffects( aShutterOpen, aOutFps );
    attributeAffects( aShutterOpen, aBBMin );
    attributeAffects( aShutterOpen, aBBMax );
    attributeAffects( aShutterOpen, aFurBBMin );
    attributeAffects( aShutterOpen, aFurBBMax );
    attributeAffects( aShutterOpen, aShowBB );
    attributeAffects( aShutterOpen, aOutUVs );
    attributeAffects( aShutterOpen, aObjects );

    attributeAffects( aShutterClose, aOutFps );
    attributeAffects( aShutterClose, aBBMin );
    attributeAffects( aShutterClose, aBBMax );
    attributeAffects( aShutterClose, aFurBBMin );
    attributeAffects( aShutterClose, aFurBBMax );
    attributeAffects( aShutterClose, aShowBB );
    attributeAffects( aShutterClose, aOutUVs );
    attributeAffects( aShutterClose, aObjects );

    attributeAffects( aFurBBPad, aFurBBMin );
    attributeAffects( aFurBBPad, aFurBBMax );

    return MS::kSuccess;

}

MStatus bb_alembicArchiveShape::GetPointsFromAbc()
{

  MStatus status;

  bb_alembicArchiveShape* nonConstThis = const_cast<bb_alembicArchiveShape*> (this);
  AlembicArchiveGeom* geom = nonConstThis->geometry();
//
//  vector<AtPoint> vertices;
//  vector<AtUInt32> vidxs;
//  vector<unsigned int> nsides;

  MString abcfile = geom->filename;

  MObject this_object = thisMObject();

  MPlug plug(this_object, aTime);
  plug.getValue(fGeometry.frame);

  plug.setAttribute(aTimeOffset);
  plug.getValue(fGeometry.frameOffset);

  if (abcfile != "")
  {
		// test the file that we can read it

		Alembic::Abc::IArchive archive = m_scene->getArchive();

		// walk the abc file

		Alembic::Abc::IObject start = archive.getTop();

		walk(start);

//		geom->m_geometryList.clear();

		for (std::vector<Alembic::Abc::IObject>::iterator i = outIObjList.begin(); i != outIObjList.end(); i++)
		{
			//get the type for this object

			Alembic::Abc::IObject this_object = *i;

			//cout << "object :: " <<  this_object.getFullName() << endl;

			// get the type of object and if it is compatible add it to the display list

//			// first if this is a polymesh
//            if (Alembic::AbcGeom::IPolyMesh::matches( this_object.getHeader() ))
//            {
//               geom->m_geometryList.push_back(new AlembicPolymeshGeometry(this_object, geom->bbox));
//            }
//            else if (Alembic::AbcGeom::ISubD::matches( this_object.getHeader() ))
//            {
//                geom->m_geometryList.push_back(new AlembicSubDGeometry(this_object, geom->bbox));
//            }
//            else if (Alembic::AbcGeom::IXform::matches( this_object.getHeader() ))
//            {
//
//            }

		}

  }

}

AlembicArchiveGeom* bb_alembicArchiveShape::geometry()
{

  int tmpMode = fGeometry.mode;


  MObject this_object = thisMObject();
  MPlug plug(this_object,aAbcFile);
  plug.getValue(fGeometry.filename);

  plug.setAttribute(aMode);
  plug.getValue(fGeometry.mode);


  // if the mode is not bounding box populate the geometry infomation

//  if (fGeometry.mode != 0 || !LoadBoundingBox())
//  {
//     GetPointsFromAbc();
//  }

  return &fGeometry;

}


std::string bb_alembicArchiveShape::getSceneKey(bool proxy) const
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

MStatus bb_alembicArchiveShape::emitCache(float relativeFrame)  {

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

	///////////// Get time with offset ///////////////////////

	MPlug timePlug( thisNode, aTime );
	MTime time;
	timePlug.getValue( time );
	//cerr << "time :: " <<  time << endl;

	MPlug timeOffsetPlug( thisNode, aTimeOffset );
	MTime timeOffset;
	timeOffsetPlug.getValue( timeOffset );
	//cerr << "timeOffset :: " <<  timeOffset << endl;

	//float o_time=time+timeOffset;
	float o_time=time.as(MTime::kFilm)+timeOffset.as(MTime::kFilm);
	//float o_time=relativeFrame+timeOffset.as(MTime::kFilm);
	//cerr << "o_time :: " <<  o_time << endl;



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
	m_bbmode = 1;
	MBoundingBox bb = boundingBox();

	//MPlug geoBBPlug( thisNode, aBB );
	MVector bmin,bmax;
	bmin=bb.min();
	bmax=bb.max();

	MFloatVector bminf(bmin);
	MFloatVector bmaxf(bmax);
	RtBound bound = { bminf.x, bmaxf.x, bminf.y, bmaxf.y, bminf.z, bmaxf.z } ;

	m_bbmode = 0;
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
	argsString += o_time;

 	//cerr << "argsString :: " <<  argsString << endl;

	//st = createProgramStrings(geomPlug,progStrings, detailRanges, lodSetNames, blurSamples, relativeFrame) ;
	progData[0] = "AlembicRiProcedural";
	progData[1] = argsString.asChar();

	RiAttributeBegin();
	RiProcedural(progData,bound,RiProcDynamicLoad,RiProcFree);
	RiAttributeEnd();


	return MS::kSuccess;

}


// UI


bb_alembicArchiveShapeUI::bb_alembicArchiveShapeUI()
{
}
bb_alembicArchiveShapeUI::~bb_alembicArchiveShapeUI()
{
}

void* bb_alembicArchiveShapeUI::creator()
{
   return new bb_alembicArchiveShapeUI();
}

//
// simple bounding box creator
void bb_alembicArchiveShapeUI::drawABox(const MVector &bmin, const MVector &bmax,bool poly=false) const {


  MStatus st;

  // Initialize GL function table first time through
  static MGLFunctionTable *gGLFT = NULL;
  if (gGLFT == NULL)
     gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();

  if (poly){
      gGLFT->glBegin(GL_QUADS);
      gGLFT->glShadeModel( GL_FLAT );

    } else {
        gGLFT->glBegin( MGL_LINE_STRIP );
    }
  float topLeftFront[3] = { float(bmin.x),float(bmax.y),float(bmax.z) };
  float topRightFront[3] = { float(bmax.x),float(bmax.y),float(bmax.z) };
  float topLeftBack[3] = { float(bmin.x),float(bmax.y),float(bmin.z) };
  float topRightBack[3] = { float(bmax.x),float(bmax.y),float(bmin.z) };

  float bottomLeftFront[3] = { float(bmin.x),float(bmin.y),float(bmax.z) };
  float bottomRightFront[3] = { float(bmax.x),float(bmin.y),float(bmax.z) };
  float bottomLeftBack[3] = { float(bmin.x),float(bmin.y),float(bmin.z) };
  float bottomRightBack[3] = { float(bmax.x),float(bmin.y),float(bmin.z) };

                ////////////////// BASE
  gGLFT->glNormal3f(0, -1, 0);
  gGLFT->glVertex3fv(bottomLeftFront);
  gGLFT->glVertex3fv(bottomLeftBack);
  gGLFT->glVertex3fv(bottomRightBack);
  gGLFT->glVertex3fv(bottomRightFront);

                /////////////////// FRONT

  gGLFT->glNormal3f(0, 0, 1);
  gGLFT->glVertex3fv(bottomLeftFront);
  gGLFT->glVertex3fv(topLeftFront);
  gGLFT->glVertex3fv(topRightFront);
  gGLFT->glVertex3fv(bottomRightFront);

              /////////////////// RIGHT

gGLFT->glNormal3f(-1, 0, 0);
gGLFT->glVertex3fv(bottomRightBack);
gGLFT->glVertex3fv(topRightBack);
gGLFT->glVertex3fv(topRightFront);


                ////////////////// TOP

  gGLFT->glNormal3f(0, 1, 0);
  gGLFT->glVertex3fv(topRightBack);
  gGLFT->glVertex3fv(topLeftBack);
  gGLFT->glVertex3fv(topLeftFront);

                /////////////////// LEFT

  gGLFT->glNormal3f(-1, 0, 0);
  gGLFT->glVertex3fv(bottomLeftFront);
  gGLFT->glVertex3fv(bottomLeftBack);
  gGLFT->glVertex3fv(topLeftBack);

                /////////////////// BACK -- correct

  gGLFT->glNormal3f(0, 1, 0);
  gGLFT->glVertex3fv(topRightBack);
  gGLFT->glVertex3fv(bottomRightBack);
  gGLFT->glVertex3fv(bottomLeftBack);

  gGLFT->glEnd();

  if (!poly){
  // make the corners big points
      gGLFT->glPushAttrib(GL_CURRENT_BIT);
      gGLFT->glPointSize(5);
      gGLFT->glBegin( GL_POINTS );
      gGLFT->glVertex3f(float(bmin.x),float(bmin.y),float(bmin.z));
      gGLFT->glVertex3f(float(bmin.x),float(bmin.y),float(bmax.z));
      gGLFT->glVertex3f(float(bmin.x),float(bmax.y),float(bmin.z));
      gGLFT->glVertex3f(float(bmin.x),float(bmax.y),float(bmax.z));
      gGLFT->glVertex3f(float(bmax.x),float(bmin.y),float(bmin.z));
      gGLFT->glVertex3f(float(bmax.x),float(bmin.y),float(bmax.z));
      gGLFT->glVertex3f(float(bmax.x),float(bmax.y),float(bmin.z));
      gGLFT->glVertex3f(float(bmax.x),float(bmax.y),float(bmax.z));
      gGLFT->glEnd();
  }
  gGLFT->glPopAttrib();

}


void bb_alembicArchiveShapeUI::getDrawRequests(const MDrawInfo & info, bool /*objectAndActiveOnly*/,
      MDrawRequestQueue & queue)
{

   // The draw data is used to pass geometry through the
   // draw queue. The data should hold all the information
   // needed to draw the shape.
   //
   MDrawData data;
   MDrawRequest request = info.getPrototype(*this);
   bb_alembicArchiveShape* archiveShape = (bb_alembicArchiveShape*) surfaceShape();
   AlembicArchiveGeom* geom = archiveShape->geometry();
   getDrawData(geom, data);
   request.setDrawData(data);

   //std::cout << "bb_alembicArchiveShapeUI::getDrawRequests" << std::endl;

   // Are we displaying meshes?
   if (!info.objectDisplayStatus(M3dView::kDisplayMeshes))
      return;

   getDrawRequestsWireFrame(request, info);
   queue.add(request);

}

void bb_alembicArchiveShapeUI::getDrawRequestsWireFrame(MDrawRequest& request, const MDrawInfo& info)
{
   request.setToken(kDrawBoundingBox);
   M3dView::DisplayStatus displayStatus = info.displayStatus();
   M3dView::ColorTable activeColorTable = M3dView::kActiveColors;
   M3dView::ColorTable dormantColorTable = M3dView::kDormantColors;

   //std::cout << "bb_alembicArchiveShapeUI::getDrawRequestsWireFrame" << std::endl;

   switch (displayStatus)
   {
   case M3dView::kLead:
      request.setColor(LEAD_COLOR, activeColorTable);
      break;
   case M3dView::kActive:
      request.setColor(ACTIVE_COLOR, activeColorTable);
      break;
   case M3dView::kActiveAffected:
      request.setColor(ACTIVE_AFFECTED_COLOR, activeColorTable);
      break;
   case M3dView::kDormant:
      request.setColor(DORMANT_COLOR, dormantColorTable);
      break;
   case M3dView::kHilite:
      request.setColor(HILITE_COLOR, activeColorTable);
      break;
   default:
      break;
   }

}
void bb_alembicArchiveShapeUI::draw( const MDrawRequest & request, M3dView & view ) const
{
    MStatus st;

    // Initialize GL function table first time through
    static MGLFunctionTable *gGLFT = NULL;
    if (gGLFT == NULL)
       gGLFT = MHardwareRenderer::theRenderer()->glFunctionTable();

    int dList = gGLFT->glGenLists(2);

    MDrawData data = request.drawData();

    bb_alembicArchiveShape* archiveShape =  (bb_alembicArchiveShape*)surfaceShape();
    AlembicArchiveGeom * geom = (AlembicArchiveGeom*) data.geometry();

    MBoundingBox bb = archiveShape->boundingBox();
    MVector bmin,bmax;
    bmin=bb.min();
    bmax=bb.max();

    MColor col;

    view.beginGL();

    std::cout << "bb_alembicArchiveShapeUI::draw -> mode:" << request.token() << std::endl;


    //archiveShape->setHolderTime();

    // draw the bounding box
    drawABox(bmin, bmax,false);

    // init gl shaders - DISABLED FOR NOW - not even sure if we can do this here
    // glshader.init((char *)vshader, (char *)fshader);



//        gGLFT->glEnable (GL_BLEND);
//        gGLFT->glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



    // Now for the geo ....

    // get the mode for the draw

    int gl_mode=1;

    //MPlug glPlug(archiveShape->aShowGL);
    //bool doGL = glPlug.asBool() ;

    // draw the "scene" from the alembic file



/*
    bool proxy = false;

    MPlug plug =  fn.findPlug( aShowProxy );
    plug.getValue( proxy );
*/

    gGLFT->glNewList(dList, MGL_COMPILE);
//	gGLFT->glShadeModel( MGL_SMOOTH );
    gGLFT->glPushAttrib(MGL_ALL_ATTRIB_BITS);
//	gGLFT->glPushAttrib(MGL_CURRENT_BIT);
	gGLFT->glEnable(MGL_POLYGON_OFFSET_FILL);
	gGLFT->glEnable(MGL_LIGHTING);
//	gGLFT->glEnable(MGL_COLOR_MATERIAL);

	gGLFT->glColor4f(0.5f, 0.5f, 0.5f, 1.0f);

    std::string sceneKey = archiveShape->getSceneKey(false);
    if (archiveShape->abcSceneManager.hasKey(sceneKey) && gl_mode == 1 ){
    	SimpleAbcViewer::ScenePtr this_scene = archiveShape->abcSceneManager.getScene(sceneKey);
    	this_scene->setGLFTable(gGLFT);
    	this_scene->draw(archiveShape->abcSceneState);
    }

    gGLFT->glEnd();
    gGLFT->glPopAttrib();

    gGLFT->glEndList();

    gGLFT->glCallList(dList);

    view.endGL();

    // update scene if needed
    //if (m_abcdirty) updateAbc(this);

//    std::cout << "draw" << std::endl;
//    timer::timer TIMER;
//    TIMER.start();

/*
    if (status == 8) {
        col = MColor(0.8,1.0,0.8);
    } else if (status == 0) {
        col = MColor(0.25,1.0,0.5);
    } else {
        col = MColor(0.7,0.5,0.5);
    }
*/
    /*
    switch (status)  // From M3dView::DisplayStatus status  in the draw() command
    {
                case M3dView::kLead:
                        col= MColor(0.26, 1.0, 0.64)  ;         // maya green
                        bColOverride = true;
                        break ;

                case M3dView::kActive:
                        col = MColor(1.0, 1.0, 1.0)  ;          // maya white
                        bColOverride = true;
                        break ;

                case M3dView::kActiveAffected:
                        col = MColor(0.78, 1.0, 0.78)  ;        // maya magenta
                        bColOverride = true;
                        break ;

                case M3dView::kTemplate:
                        col = MColor(0.47, 0.47, 0.47)  ;       // maya template gray
                        bColOverride = true;
                        break ;

                case M3dView::kActiveTemplate:
                        col = MColor(1.0, 0.47, 0.47)  ;        // maya selected template pink
                        bColOverride = true;
                        break ;

                default:
                        //col = MColor(0.7,0.5,0.5);
                        col = MColor(0.1, 0.2, 0.7) ;   // else set color as desired
    }
        */
/*
    // this makes a copy of the current openGL settings so that anything
    // we change will not affect anything else maya draws afterwards.
    glPushAttrib( GL_ALL_ATTRIB_BITS );

    archiveShape->setHolderTime();

    // glUseProgram(glshader.getProgram());
*/
//    glColor3f(col.r,col.g,col.b);

    //MFnDagNode fn( thisNode );



}

/* override */
bool bb_alembicArchiveShapeUI::select( MSelectInfo &selectInfo, MSelectionList &selectionList,
                                        MPointArray &worldSpaceSelectPts ) const
//
// Description:
//
//     Main selection routine
//
// Arguments:
//
//     selectInfo           - the selection state information
//     selectionList        - the list of selected items to add to
//     worldSpaceSelectPts  -
//
{

  MSelectionMask priorityMask(MSelectionMask::kSelectObjectsMask);
  MSelectionList item;
  item.add(selectInfo.selectPath());
  MPoint xformedPt;
  selectInfo.addSelection(item, xformedPt, selectionList, worldSpaceSelectPts, priorityMask, false);
  return true;
}
