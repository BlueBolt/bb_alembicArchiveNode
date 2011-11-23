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

#include <delightOutputFlock.h>
#include <flockShape.h>


//	static
void* delightOutputFlock::creator()
{
	return new delightOutputFlock();
}

MSyntax delightOutputFlock::newSyntax()
{
	MSyntax syn;

	syn.setObjectType(MSyntax::kSelectionList, 0, 1);
	syn.useSelectionAsDefault(false);
	
	return syn;
}

MStatus delightOutputFlock::doIt( const MArgList& args )
{
	MStatus st;
	MString method("delightOutputFlock::doIt");
	
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
	
	if (!(flockShapeNode)) {
		displayError("No flockShape node specified");
		return MS::kUnknownParameter;
	}

	st = flockShapeNode->removeCache();	er;
	st = flockShapeNode->addSlice(1.0f);er;
	st = flockShapeNode->emitCache();er;
	st = flockShapeNode->removeCache();	er;

	return MS::kSuccess;
}

