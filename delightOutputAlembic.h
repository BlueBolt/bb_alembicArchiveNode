#ifndef	__delightOutputFlock_H__
#define	__delightOutputFlock_H__

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>




/////////////////////////////////////////
//
//	delightOutputFlock
//		: MPxCommand
//
class delightOutputFlock : public MPxCommand

{
	public:
	
	delightOutputFlock() {}
	virtual ~delightOutputFlock() {}
	
	MStatus doIt( const MArgList& args );
	
	static void* creator();
	
   static MSyntax      newSyntax();	

};

#endif	//	!__delightOutputFlock_H__

