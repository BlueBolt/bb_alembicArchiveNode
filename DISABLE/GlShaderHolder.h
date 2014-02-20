#ifndef _GlShaderHolder_h_
#define _GlShaderHolder_h_

//#include <GL/glx.h>
#include <GL/glew.h>
#include <maya/MPxLocatorNode.h>
//#include <GL/glut.h>

class GlShaderHolder
{
    public:
        void init(char* vs, char* fs);
        
        GLuint getProgram() { return m_p; }
        
    
    private:
    
        GLuint m_v;
        GLuint m_f;
        GLuint m_p;

};


#endif
