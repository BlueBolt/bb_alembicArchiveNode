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

#ifndef _SimpleAbcViewer_Scene_h_
#define _SimpleAbcViewer_Scene_h_

#include "Foundation.h"
//#include "GLCamera.h"
#include "Drawable.h"

#include <boost/shared_ptr.hpp>

namespace SimpleAbcViewer {

//-*****************************************************************************
struct SceneState
{
//    GLCamera cam;
    float pointSize;
};

//-*****************************************************************************
class Scene
{
public:
    //! Load a scene from the alembic archive given by the filename.
    //! ...
    Scene( const std::string &abcFileName, const std::string &objectPath);

    //! Return the filename of the archive
    //! ...
    const std::string &getFileName() const { return m_fileName; }

    //! Return the top object object
    //! ...
    const IObject &getTopObject() const { return m_topObject; }

    //! Return the archive
    //! ...
    const IArchive &getArchive() const { return m_archive; }

    //! Return the min time, in seconds.
    //! ...
    chrono_t getMinTime() const { return m_minTime; }

    //! Return the max time, in seconds.
    //! ...
    chrono_t getMaxTime() const { return m_maxTime; }

    //! Return whether it's animated.
    //! ...
    bool isConstant() const { return m_minTime >= m_maxTime; }

    //! Cause the drawable state to be loaded to the given time.
    //! ...
    void setTime( chrono_t newTime );

    //! Return the bounds at the current time.
    //! ...
    Box3d getBounds() const { return m_bounds; }

    //! This draws, assuming a camera matrix has already been set.
    //! ...
    void draw( SceneState &s_state );
    
protected:
    std::string m_fileName;
    std::string m_objectPath;
    IArchive m_archive;
    IObject m_topObject;

    chrono_t m_minTime;
    chrono_t m_maxTime;
    Box3d m_bounds;

    DrawablePtr m_drawable;
};

typedef boost::shared_ptr<Scene> ScenePtr;

} // End namespace SimpleAbcViewer

#endif
