//-*****************************************************************************
//
// Copyright (c) 2009-2010,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#include "Scene.h"
#include "IObjectDrw.h"
#include "PathUtil.h"

#include "boost/foreach.hpp"

//-*****************************************************************************
namespace SimpleAbcViewer {

//-*****************************************************************************
void setMaterials( float o, bool negMatrix = false )
{
    if ( negMatrix )
    {
        GLfloat mat_front_diffuse[] = { 0.1 * o, 0.1 * o, 0.9 * o, o };
        GLfloat mat_back_diffuse[] = { 0.9 * o, 0.1 * o, 0.9 * o, o };

        GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
        GLfloat mat_shininess[] = { 100.0 };
//        GLfloat light_position[] = { 20.0, 20.0, 20.0, 0.0 };

        glClearColor( 0.0, 0.0, 0.0, 0.0 );
        glMaterialfv( GL_FRONT, GL_DIFFUSE, mat_front_diffuse );
        glMaterialfv( GL_FRONT, GL_SPECULAR, mat_specular );
        glMaterialfv( GL_FRONT, GL_SHININESS, mat_shininess );

        glMaterialfv( GL_BACK, GL_DIFFUSE, mat_back_diffuse );
        glMaterialfv( GL_BACK, GL_SPECULAR, mat_specular );
        glMaterialfv( GL_BACK, GL_SHININESS, mat_shininess );    
    }
    else
    {

        GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
        GLfloat mat_shininess[] = { 100.0 };
//        GLfloat light_position[] = { 20.0, 20.0, 20.0, 0.0 };
        GLfloat mat_front_emission[] = {0.0, 0.0, 0.0, 0.0 };
        GLfloat mat_back_emission[] = {o, 0.0, 0.0, o };

        glClearColor( 0.0, 0.0, 0.0, 0.0 );
        glMaterialfv( GL_FRONT, GL_EMISSION, mat_front_emission );
        glMaterialfv( GL_FRONT, GL_SPECULAR, mat_specular );
        glMaterialfv( GL_FRONT, GL_SHININESS, mat_shininess );

        glMaterialfv( GL_BACK, GL_EMISSION, mat_back_emission );
        glMaterialfv( GL_BACK, GL_SPECULAR, mat_specular );
        glMaterialfv( GL_BACK, GL_SHININESS, mat_shininess );    

        glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
        glEnable(GL_COLOR_MATERIAL);
    }
}

//-*****************************************************************************
// SCENE CLASS
//-*****************************************************************************

MGLFunctionTable* Scene::g_GLFT = 0;

//-*****************************************************************************
Scene::Scene( const std::string &abcFileName, const std::string &objectPath )
  : m_fileName( abcFileName )
  , m_objectPath( objectPath )
  , m_minTime( ( chrono_t )FLT_MAX )
  , m_maxTime( ( chrono_t )-FLT_MAX )
{        
    boost::timer Timer;

   std::cout << "Beginning to open archive: " << abcFileName << std::endl;

    Alembic::AbcCoreFactory::IFactory factory;
    factory.setPolicy(Alembic::Abc::ErrorHandler::kQuietNoopPolicy);

    m_archive = IArchive( factory.getArchive(abcFileName) );
    m_topObject = IObject( m_archive, kTop );
    
   std::cout << "Opened archive and top object, creating drawables."
             << std::endl;

   std::cout << "ObjectPath " << objectPath
             << std::endl;


    // try to walk to the path
    PathList path;
    TokenizePath( objectPath, path );

    m_drawable.reset( new IObjectDrw( m_topObject, false, path ) );

    if ( !path.empty() ) //walk the entire scene
    {
        const ObjectHeader *nextChildHeader = &m_topObject.getHeader();
        IObject nextParentObject = m_topObject;

        BOOST_FOREACH(std::string &childName,path)
        {
            nextChildHeader = nextParentObject.getChildHeader( childName );
            if ( nextChildHeader == NULL ) {
                break;
            } else {
                nextParentObject = IObject( nextParentObject, childName );
            }   
        }

        if ( nextChildHeader != NULL )
        {
            m_topObject = nextParentObject;
        }
    }

//    m_drawable.reset( new IObjectDrw( m_topObject, false ) );
    ABCA_ASSERT( m_drawable->valid(),
                 "Invalid drawable for archive: " << abcFileName );

   std::cout << "Created drawables, getting time range." << std::endl;
    m_minTime = m_drawable->getMinTime();
    m_maxTime = m_drawable->getMaxTime();

    if ( m_minTime <= m_maxTime )
    {
       std::cout << "\nMin Time: " << m_minTime << " seconds " << std::endl
                 << "Max Time: " << m_maxTime << " seconds " << std::endl
                 << "\nLoading min time." << std::endl;
        m_drawable->setTime( m_minTime );
    }
    else
    {
       std::cout << "\nConstant Time." << std::endl
                 << "\nLoading constant sample." << std::endl;
        m_minTime = m_maxTime = 0.0;
        m_drawable->setTime( 0.0 );
    }

    ABCA_ASSERT( m_drawable->valid(),
                 "Invalid drawable after reading start time" );

   std::cout << "Done opening archive. Elapsed time: "
            << Timer.elapsed() << " seconds." << std::endl;

    // Bounds have been formed!
    m_bounds = m_drawable->getBounds();
   std::cout << "Bounds at min time: " << m_bounds.min << " to "
             << m_bounds.max << std::endl;


}



//-*****************************************************************************
void Scene::setTime( chrono_t iSeconds )
{
    ABCA_ASSERT( m_archive && m_topObject &&
                 m_drawable && m_drawable->valid(),
                 "Invalid Scene: " << m_fileName );

    if ( m_minTime <= m_maxTime )
    {
        
        m_drawable->setTime( iSeconds );
        ABCA_ASSERT( m_drawable->valid(),
                     "Invalid drawable after setting time to: "
                     << iSeconds );
    }
    
    m_bounds = m_drawable->getBounds();
}

void Scene::setGLFTable(MGLFunctionTable* table)
{
   g_GLFT = table;
}

//-*****************************************************************************
void Scene::draw( SceneState &s_state )
{
    ABCA_ASSERT( m_archive && m_topObject &&
                 m_drawable && m_drawable->valid(),
                 "Invalid Scene: " << m_fileName );

//    glDrawBuffer( GL_BACK );
//    s_state.cam.apply();

//    glEnable( GL_LIGHTING );
//    setMaterials( 1.0, true );
    
    // Get the matrix
    M44d currentMatrix;
    glGetDoublev( GL_MODELVIEW_MATRIX, ( GLdouble * )&(currentMatrix[0][0]) );
    
    DrawContext dctx;
    dctx.setWorldToCamera( currentMatrix );
    dctx.setPointSize( s_state.pointSize );

    m_drawable->draw( dctx );
    
//    glutSwapBuffers();
}

} // End namespace SimpleAbcViewer
