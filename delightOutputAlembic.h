#ifndef	__delightOutputAlembic_H__
#define	__delightOutputAlembic_H__

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>




/////////////////////////////////////////
//
//	delightOutputAlembic
//		: MPxCommand
//
class delightOutputAlembic : public MPxCommand

{
	public:
	
	delightOutputAlembic() {}
	virtual ~delightOutputAlembic() {}
	
	MStatus doIt( const MArgList& args );
	
	static void* creator();
	
   static MSyntax      newSyntax();	

};

#endif	//	!__delightOutputAlembic_H__

