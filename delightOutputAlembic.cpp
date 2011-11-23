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

#include <delightOutputAlembic.h>
#include <alembicArchiveNode.h>


//	static
void* delightOutputAlembic::creator()
{
	return new delightOutputAlembic();
}

MSyntax delightOutputAlembic::newSyntax()
{
	MSyntax syn;

	syn.setObjectType(MSyntax::kSelectionList, 0, 1);
	syn.useSelectionAsDefault(false);
	
	return syn;
}

MStatus delightOutputAlembic::doIt( const MArgList& args )
{
	MStatus st;
	MString method("delightOutputAlembic::doIt");
	
	// get the node if one is specified
	MSelectionList list;
	MArgDatabase  argData(syntax(), args);
	argData.getObjects(list);
	MItSelectionList iter( list, MFn::kPluginDependNode);
	MObject  rigInstObject;
	MDagPath rigInstDagPath;
	MFnDependencyNode rigInstFn;
	alembicArchiveNode * alembicArchiveNodeObject = 0;
	MString fullPathName;
	
	for (;! iter.isDone() ; iter.next()) {
		iter.getDependNode( rigInstObject );
		rigInstFn.setObject( rigInstObject );
		if (rigInstFn.typeId() == alembicArchiveNode::id) {
			iter.getDagPath( rigInstDagPath );
			fullPathName = rigInstDagPath.fullPathName();
			alembicArchiveNodeObject =  (alembicArchiveNode*)rigInstFn.userNode();
			break;
		}
	}
	
	if (!(alembicArchiveNodeObject)) {
		displayError("No alembicArchiveNode node specified");
		return MS::kUnknownParameter;
	}

	st = alembicArchiveNodeObject->removeCache();	er;
	st = alembicArchiveNodeObject->addSlice(1.0f);er;
	st = alembicArchiveNodeObject->emitCache();er;
	st = alembicArchiveNodeObject->removeCache();	er;

	return MS::kSuccess;
}

