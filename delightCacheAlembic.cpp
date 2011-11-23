#include <maya/MPlugArray.h>
#include <maya/MTypeId.h>
#include <maya/MFloatVectorArray.h>

#include <maya/MItDag.h>
#include <maya/MItSelectionList.h>
#include <maya/MIOStream.h>
#include <math.h>
#include <maya/MArgDatabase.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagPath.h>

#include <delightCacheFlock.h>
#include <flockShape.h>


//	static
void* delightCacheFlock::creator()
{
	return new delightCacheFlock();
}

MSyntax delightCacheFlock::newSyntax()
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




MStatus delightCacheFlock::doIt( const MArgList& args )
{
	MStatus st;
	MString method("delightCacheFlock::doIt");
	
	// get the node if one is specified
	MSelectionList list;
	MArgDatabase  argData(syntax(), args);
	argData.getObjects(list);
	MItSelectionList iter( list, MFn::kPluginDependNode);
	MObject  rigInstObject;
	MDagPath rigInstDagPath;
	MFnDependencyNode rigInstFn;
	flockShape * flockShapeNode = 0;
	MString fullPathName;
	
	for (;! iter.isDone() ; iter.next()) {
		iter.getDependNode( rigInstObject );
		rigInstFn.setObject( rigInstObject );
		if (rigInstFn.typeId() == flockShape::id) {
			iter.getDagPath( rigInstDagPath );
			fullPathName = rigInstDagPath.fullPathName();
			flockShapeNode =  (flockShape*)rigInstFn.userNode();
			break;
		}
	}
	
	if (argData.isFlagSet(kAddFlag)) {
		if (!(flockShapeNode)) {
			displayError("Object not in cache");
			return MS::kUnknownParameter;
		}
		if (!(argData.isFlagSet(kSampleTimeFlag))){
			displayError("Need to specify a sample num with -st");
			return MS::kUnknownParameter;
		}
		double dsampleTime;
		float sampleTime;
		st= argData.getFlagArgument (kSampleTimeFlag, 0, dsampleTime);ert;
		sampleTime = float(dsampleTime);
		if (flockShapeNode->hasCache(sampleTime)){	
			displayError("Object and sample already in cache: "+fullPathName +" " + sampleTime);
			return MS::kUnknownParameter;
		}	else {
			st = flockShapeNode->addSlice(sampleTime);
		} 
		return st;
	}

	if (argData.isFlagSet(kEmitFlag)) {
		if (!(flockShapeNode)) {
			displayError("Object not in cache");
			return MS::kUnknownParameter;
		}
		
		if (argData.isFlagSet(kSampleTimeFlag)){
			double sampleTime;
			st= argData.getFlagArgument (kSampleTimeFlag, 0, sampleTime);ert;
			st = flockShapeNode->emitCacheSlice(float(sampleTime));
		} else {
			double relativeTime;
			if (argData.isFlagSet(kRelativeTimeFlag)){
				st= argData.getFlagArgument (kRelativeTimeFlag, 0, relativeTime);er;
			} else {
				st = MGlobal::executeCommand("delightRenderState -qf",relativeTime);er;
			}
			st = flockShapeNode->emitCache(float(relativeTime));				
		}

		return st;
	}
	
	

	if (argData.isFlagSet(kRemoveFlag)) {
		if (!(flockShapeNode)) {
			displayError("Need to specify a flockShape");
			return MS::kUnknownParameter;
		}
		st = flockShapeNode->removeCache();
		return st;
	}

	
	if (argData.isFlagSet(kFlushFlag)) {
		MItDag dagIter( MItDag::kDepthFirst, MFn::kPluginDependNode, &st);ert;
		for ( ;!dagIter.isDone();dagIter.next()){
			rigInstObject = dagIter.currentItem();
			rigInstFn.setObject( rigInstObject );
			if (rigInstFn.typeId() == flockShape::id) {
				flockShapeNode =  (flockShape*)rigInstFn.userNode();
				st = flockShapeNode->removeCache();
			}
		}
		return MS::kSuccess;
	}


	if (argData.isFlagSet(kListFlag)) {

		MItDag dagIter( MItDag::kDepthFirst, MFn::kPluginDependNode, &st);ert;
		for ( ;!dagIter.isDone();dagIter.next()){
			rigInstObject = dagIter.currentItem();
			rigInstFn.setObject( rigInstObject );
			if (rigInstFn.typeId() == flockShape::id) {
				flockShapeNode =  (flockShape*)rigInstFn.userNode();
				if (flockShapeNode->hasCache()) {
					appendToResult(dagIter.fullPathName());
				}

			}
		}
		return MS::kSuccess;
	}
	
	
	if (argData.isFlagSet(kInfoFlag)) {
		MItDag dagIter( MItDag::kDepthFirst, MFn::kPluginDependNode, &st);ert;
		for ( ;!dagIter.isDone();dagIter.next()){
			rigInstObject = dagIter.currentItem();
			rigInstFn.setObject( rigInstObject );
			if (rigInstFn.typeId() == flockShape::id) {
				flockShapeNode =  (flockShape*)rigInstFn.userNode();
				cerr << dagIter.fullPathName() << endl;;
				if (flockShapeNode->hasCache()) {
					cerr << "Cache  Exists" << endl;
					flockShapeNode->cacheInfo();
				} else {
					cerr << "Cache Does Not Exist" << endl;
				}	
			}
		}
		return MS::kSuccess;
	}
	return MS::kSuccess;
}

