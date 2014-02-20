/**
(c)2011 Bluebolt Ltd.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
* Neither the name of BlueBolt nor the names of
its contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Author:Ashley retallack - ashley-r@blue-bolt.com

File:delightCacheAlembic.h

Maya Command: delightCacheAlembic

**/

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

