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
	
	ofstream f("/disk1/tmp/debug_alembicArchive.txt", ios::app);
	f << "Args: " << args.length() << endl;
	  for(int i = 0; i < args.length(); ++i)
	  {
	    MString str;
	    args.get(i, str);
	    f << str.asChar() << endl;
	  }
	  f.close();
	
	if (!(alembicArchiveNodeObject)) {
		displayError("No alembicArchiveNode node specified");
		return MS::kUnknownParameter;
	}

	return MS::kSuccess;
}

