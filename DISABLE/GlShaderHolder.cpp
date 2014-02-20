#include "GlShaderHolder.h"
#include <iostream>

void GlShaderHolder::init(char* vs, char* fs)
{
    std::cout << "pinit0" << std::endl;
    m_v = glCreateShader(GL_VERTEX_SHADER);
    m_f = glCreateShader(GL_FRAGMENT_SHADER);
    std::cout << "pinit" << std::endl;

    const char * ff = fs;
    const char * vv = vs;

    glShaderSource(m_v, 1, &vv,NULL);
    glShaderSource(m_f, 1, &ff,NULL);
    std::cout << "pinit2" << std::endl;

    glCompileShader(m_v);
    glCompileShader(m_f);
    std::cout << "pinit3" << std::endl;

    m_p = glCreateProgram();
    glAttachShader(m_p,m_f);
    glAttachShader(m_p,m_v);
    std::cout << "pinit4" << std::endl;

    glLinkProgram(m_p);
}
