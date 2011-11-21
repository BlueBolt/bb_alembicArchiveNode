# Location where rules.BUILDTYPE -files are stored. 
INCLUDES=../RULES
# Use 'default' is BUILDTYPE is undefined.
BUILDTYPE ?= maya2011-alembic-x86_64
# Include the rules
include $(INCLUDES)/rules.$(BUILDTYPE)

# This is the location for per-project configuration
# ------------------------------------------------->
PROJECT		= $(AUTO_PROJECT)
OBJECTS		= 

# <-------------------------------------------------

TARGET		= $(PROJECT)-$(MAYA_VERSION).$(DSO)

$(TARGET):	$(AUTO_OBJECTS)
		$(CXX) $(LDFLAGS) $(AUTO_OBJECTS) $(OBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

animaAlembicHolderNode.o:	glsl/abc.vert.xxd glsl/abc.frag.xxd animaAlembicHolderNode.cpp

glsl/abc.vert.xxd: glsl/abc.vert
		xxd -i < glsl/abc.vert > glsl/abc.vert.xxd; echo ",0" >> glsl/abc.vert.xxd
glsl/abc.frag.xxd: glsl/abc.frag
		xxd -i <glsl/abc.frag > glsl/abc.frag.xxd; echo ",0" >> glsl/abc.frag.xxd

objs:		$(OBJECTS)

clean:		
		$(RM) *.o *.so *~ $(OBJECTS) glsl/*.xxd

install:	$(TARGET)
		$(INSTALL) $(TARGET) $(INSTPATH)/$(PROJECT).$(DSO)
