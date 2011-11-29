

#ifndef errorMacros_h
#define errorMacros_h



#include <maya/MStatus.h>
 

// print error details 
#define er															\
    if (MS::kSuccess != st) {										\
        cerr << "    in File: " << __FILE__							\
			 << ", at Line: " << __LINE__ << endl;					\
        st.perror("Error");	  								\
    }
// print error details and return
#define ert															\
    if (MS::kSuccess != st) {										\
        cerr << "    in File: " << __FILE__							\
			 << ", at Line: " << __LINE__ << endl;					\
        st.perror("Error");	  								\
        return st;													\
    }

#define erv															\
    if (MS::kSuccess != st) {										\
        cerr << "    in File: " << __FILE__							\
			 << ", at Line: " << __LINE__ << endl;					\
        st.perror("Error");	  										\
        return ;													\
    }

#define er1															\
    if (MS::kSuccess != st) {										\
        cerr << "    in File: " << __FILE__							\
			 << ", at Line: " << __LINE__ << endl;					\
        st.perror("Error");			 							\
        return 1;													\
    }


#define tracer \
	cerr << "Line: " <<  __LINE__ <<  "File: " << __FILE__	 << endl;


/*
#define er	;

#define ert 	if (MS::kSuccess != st)   return st;	
*/

#endif
