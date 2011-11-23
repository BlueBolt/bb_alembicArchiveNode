#ifndef	__delightCacheFlock_H__
#define	__delightCacheFlock_H__

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
//	delightCacheFlock
//		: MPxCommand
//
class delightCacheFlock : public MPxCommand

{
	public:
	
	delightCacheFlock() {}
	virtual ~delightCacheFlock() {}
	
	MStatus doIt( const MArgList& args );
	
	static void* creator();
	
   static MSyntax      newSyntax();	

};

#endif	//	!__delightCacheFlock_H__

