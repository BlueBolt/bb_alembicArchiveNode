#ifndef	__delightCacheAlembic_H__
#define	__delightCacheAlembic_H__

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>

#define kAddFlag				"-a"
#define kAddFlagL 				"-addStep"

#define kEmitFlag				"-e"
#define kEmitFlagL 				"-emit"

#define kFlushFlag				"-f"
#define kFlushFlagL 			"-flush"

#define kListFlag				"-l"
#define kListFlagL 				"-list"

#define kRemoveFlag				"-r"
#define kRemoveFlagL 			"-remove"

#define kSampleTimeFlag			"-st"
#define kSampleTimeFlagL 		"-sampleTime"

#define kInfoFlag				"-i"
#define kInfoFlagL 				"-info"

#define kRelativeTimeFlag		"-rt"
#define kRelativeTimeFlagL 		"-relativeTime"


/////////////////////////////////////////
//
//	delightCacheAlembic
//		: MPxCommand
//
class delightCacheAlembic : public MPxCommand

{
	public:
	
	delightCacheAlembic() {}
	virtual ~delightCacheAlembic() {}
	
	MStatus doIt( const MArgList& args );
	
	static void* creator();
	
   static MSyntax      newSyntax();	

};

#endif	//	!__delightCacheAlembic_H__

