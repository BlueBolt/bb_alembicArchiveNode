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

#include "IObjectDrw.h"
#include "IPolyMeshDrw.h"
//#include "ISimpleXformDrw.h"
#include "IXformDrw.h"
//#include "IPointsDrw.h"
//#include "ISubDDrw.h"

namespace SimpleAbcViewer {

//-*****************************************************************************
IObjectDrw::IObjectDrw( IObject &iObj, bool iResetIfNoChildren, std::vector<std::string> path )
  : m_object( iObj )
  , m_minTime( ( chrono_t )FLT_MAX )
  , m_maxTime( ( chrono_t )-FLT_MAX )
  , m_currentTime( ( chrono_t )-FLT_MAX )
{
    // If not valid, just bail.
    if ( !m_object ) { return; }

    //std::cout << "IObjectDraw path: " << path << std::endl;
    if (path.size()) {
        const ObjectHeader *ohead = m_object.getChildHeader( path[0] );
        if ( ohead!=NULL ) {
            path.erase(path.begin());
            DrawablePtr dptr;
            if ( IXform::matches( *ohead ) ) {
                IXform xform( m_object, ohead->getName() );
                if ( xform ) {
                    dptr.reset( new IXformDrw( xform, path ) );
                }
            }
            if ( dptr && dptr->valid() ) {
                m_children.push_back( dptr );
                m_minTime = std::min( m_minTime, dptr->getMinTime() );
                m_maxTime = std::max( m_maxTime, dptr->getMaxTime() );
            }
        }
    }       
    else
    {   
        // IObject has no explicit time sampling, but its children may.
        size_t numChildren = m_object.getNumChildren();
        for ( size_t i = 0; i < numChildren; ++i )
        {
            const ObjectHeader &ohead = m_object.getChildHeader( i );

            // Decide what to make.
            DrawablePtr dptr;
            if ( IPolyMesh::matches( ohead ) )
            {
                IPolyMesh pmesh( m_object, ohead.getName() );
                if ( pmesh )
                {
                    dptr.reset( new IPolyMeshDrw( pmesh, path ) );
                }
            }
    /*        else if ( IPoints::matches( ohead ) )
            {
                IPoints points( m_object, ohead.getName() );
                if ( points )
                {
                    dptr.reset( new IPointsDrw( points ) );
                }
            }
            else if ( IXform::matches( ohead ) )
            {
                IXform xform( m_object, ohead.getName() );
                if ( xform )
                {
                    dptr.reset( new ISimpleXformDrw( xform ) );
                }
            }
    */        else if ( IXform::matches( ohead ) )
            {
                IXform xform( m_object, ohead.getName() );
                if ( xform )
                {
                    dptr.reset( new IXformDrw( xform, path ) );
                }
            }
    /*        else if ( ISubD::matches( ohead ) )
            {
                ISubD subd( m_object, ohead.getName() );
                if ( subd )
                {
                    dptr.reset( new ISubDDrw( subd ) );
                }
            }
    */        else
            {
                IObject object( m_object, ohead.getName() );
                if ( object )
                {
                    dptr.reset( new IObjectDrw( object, true, path ) );
                }
            }

            if ( dptr && dptr->valid() )
            {
                m_children.push_back( dptr );
                m_minTime = std::min( m_minTime, dptr->getMinTime() );
                m_maxTime = std::max( m_maxTime, dptr->getMaxTime() );
            }
        }
    }
    
    // Make the bounds empty to start
    m_bounds.makeEmpty();

    // If we have no children, just leave.
    if ( m_children.size() == 0 && iResetIfNoChildren )
    {
        m_object.reset();
    }
}

//-*****************************************************************************
IObjectDrw::~IObjectDrw()
{
    // Nothing!
}

//-*****************************************************************************
chrono_t IObjectDrw::getMinTime()
{
    return m_minTime;
}

//-*****************************************************************************
chrono_t IObjectDrw::getMaxTime()
{
    return m_maxTime;
}

//-*****************************************************************************
bool IObjectDrw::valid()
{
    return m_object.valid();
}

//-*****************************************************************************
void IObjectDrw::setTime( chrono_t iTime )
{
    if (iTime != m_currentTime)
    {
        m_currentTime = iTime;
        
        if ( !m_object ) { return; }

        // Object itself has no properties to worry about.
        m_bounds.makeEmpty();
        for ( DrawablePtrVec::iterator iter = m_children.begin();
              iter != m_children.end(); ++iter )
        {
            DrawablePtr dptr = (*iter);
            if ( dptr )
            {
                dptr->setTime( iTime );
                m_bounds.extendBy( dptr->getBounds() );
            }
        }
    }
}

//-*****************************************************************************
Box3d IObjectDrw::getBounds()
{
    return m_bounds;
}

//-*****************************************************************************
void IObjectDrw::draw( const DrawContext &iCtx )
{
    if ( !m_object ) { return; }

    for ( DrawablePtrVec::iterator iter = m_children.begin();
          iter != m_children.end(); ++iter )
    {
        DrawablePtr dptr = (*iter);
        if ( dptr )
        {
            dptr->draw( iCtx );
        }
    }
}

} // End namespace SimpleAbcViewer

