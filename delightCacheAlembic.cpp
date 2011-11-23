#include <maya/MPlugArray.h>
#include <maya/MTypeId.h>
#include <maya/MFloatVectorArray.h>

#include <maya/MItDag.h>
#include <maya/MItSelectionList.h>
#include <maya/MIOStream.h>
#include <maya/MGlobal.h>
#include <math.h>
#include <maya/MArgDatabase.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagPath.h>

#include "errorMacros.h"

#include "delightCacheAlembic.h"
#include "alembicArchiveNode.h"


//	static
void* delightCacheAlembic::creator()
{
	return new delightCacheAlembic();
}

MSyntax delightCacheAlembic::newSyntax()
{
	MSyntax syn;

	syn.addFlag(kAddFlag,kAddFlagL);
	syn.addFlag(kEmitFlag,kEmitFlagL);
	syn.addFlag(kFlushFlag,kFlushFlagL);
	syn.addFlag(kListFlag,kListFlagL);
	syn.addFlag(kRemoveFlag	,kRemoveFlagL);
	syn.addFlag(kSampleTimeFlag,kSampleTimeFlagL,MSyntax::kDouble );
	syn.addFlag(kInfoFlag	,kInfoFlagL);
	syn.addFlag(kRelativeTimeFlag,kRelativeTimeFlagL,MSyntax::kDouble );

	syn.setObjectType(MSyntax::kSelectionList, 0, 1);
	syn.useSelectionAsDefault(false);
	
	return syn;
}

MStatus delightCacheAlembic::doIt( const MArgList& args )
{
	MStatus st;
	MString method("delightCacheAlembic::doIt");
	
	// get the node if one is specified
	MSelectionList list;
	MArgDatabase  argData(syntax(), args);
	argData.getObjects(list);
	MItSelectionList iter( list, MFn::kPluginDependNode);
	MObject  rigInstObject;
	MDagPath rigInstDagPath;
	MFnDependencyNode rigInstFn;
	alembicArchiveNode * alembicArchive = 0;
	MString fullPathName;
	
	for (;! iter.isDone() ; iter.next()) {
		iter.getDependNode( rigInstObject );
		rigInstFn.setObject( rigInstObject );
		if (rigInstFn.typeId() == alembicArchiveNode::id) {
			iter.getDagPath( rigInstDagPath );
			fullPathName = rigInstDagPath.fullPathName();
			alembicArchive =  (alembicArchiveNode*)rigInstFn.userNode();
			break;
		}
	}
	
	if (argData.isFlagSet(kAddFlag)) {
		if (!(alembicArchive)) {
			displayError("Object not in cache");
			return MS::kUnknownParameter;
		}
		if (!(argData.isFlagSet(kSampleTimeFlag))){
			displayError("Need to specify a sample num with -st");
			return MS::kUnknownParameter;
		}
		double dsampleTime;
		float sampleTime;
		st= argData.getFlagArgument (kSampleTimeFlag, 0, dsampleTime);
		sampleTime = float(dsampleTime);
/*		if (alembicArchive->hasCache(sampleTime)){
			displayError("Object and sample already in cache: "+fullPathName +" " + sampleTime);
			return MS::kUnknownParamet
		}	else {
			st = alembicArchive->addSlice(sampleTime);
		} */
		return st;
	}

	if (argData.isFlagSet(kEmitFlag)) {
		if (!(alembicArchive)) {
			displayError("Object not in cache");
			return MS::kUnknownParameter;
		}
		
		if (argData.isFlagSet(kSampleTimeFlag)){
			double sampleTime;
			st= argData.getFlagArgument (kSampleTimeFlag, 0, sampleTime);
		//	st = alembicArchive->emitCacheSlice(float(sampleTime));
		} else {
			double relativeTime;
			if (argData.isFlagSet(kRelativeTimeFlag)){
				st= argData.getFlagArgument (kRelativeTimeFlag, 0, relativeTime);
			} else {
				st = MGlobal::executeCommand("delightRenderState -qf",relativeTime);
			}
			//st = alembicArchive->emitCache(float(relativeTime));
		}

		return st;
	}


	if (argData.isFlagSet(kRemoveFlag)) {
		if (!(alembicArchive)) {
			displayError("Need to specify a alembicArchiveNode");
			return MS::kUnknownParameter;
		}
		//st = alembicArchive->removeCache();
		return st;
	}

	
	if (argData.isFlagSet(kFlushFlag)) {
		MItDag dagIter( MItDag::kDepthFirst, MFn::kPluginDependNode, &st);
		for ( ;!dagIter.isDone();dagIter.next()){
			rigInstObject = dagIter.currentItem();
			rigInstFn.setObject( rigInstObject );
			if (rigInstFn.typeId() == alembicArchiveNode::id) {
				alembicArchive =  (alembicArchiveNode*)rigInstFn.userNode();
				//st = alembicArchive->removeCache();
			}
		}
		return MS::kSuccess;
	}


	if (argData.isFlagSet(kListFlag)) {

		MItDag dagIter( MItDag::kDepthFirst, MFn::kPluginDependNode, &st);
		for ( ;!dagIter.isDone();dagIter.next()){
			rigInstObject = dagIter.currentItem();
			rigInstFn.setObject( rigInstObject );
			if (rigInstFn.typeId() == alembicArchiveNode::id) {
				alembicArchive =  (alembicArchiveNode*)rigInstFn.userNode();
				/*if (alembicArchive->hasCache()) {
					appendToResult(dagIter.fullPathName());
				}*/

			}
		}
		return MS::kSuccess;
	}
	
	
	if (argData.isFlagSet(kInfoFlag)) {
		MItDag dagIter( MItDag::kDepthFirst, MFn::kPluginDependNode, &st);
		for ( ;!dagIter.isDone();dagIter.next()){
			rigInstObject = dagIter.currentItem();
			rigInstFn.setObject( rigInstObject );
			if (rigInstFn.typeId() == alembicArchiveNode::id) {
				alembicArchive =  (alembicArchiveNode*)rigInstFn.userNode();
				cerr << dagIter.fullPathName() << endl;;
				/*if (alembicArchive->hasCache()) {
					cerr << "Cache  Exists" << endl;
					alembicArchive->cacheInfo();
				} else {
					cerr << "Cache Does Not Exist" << endl;
				}*/
			}
		}
		return MS::kSuccess;
	}
	return MS::kSuccess;
}

