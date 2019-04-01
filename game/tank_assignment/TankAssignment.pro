TEMPLATE = app

#Executable Name
TARGET = TankAssignment
CONFIG = debug

#Destination
DESTDIR = .
OBJECTS_DIR = ./build/

HEADERS	+= 	../common/Shader.h	    	\	
		../common/Vector.h		        \	
		../common/Matrix.h		        \
		../common/Mesh.h		        \
        ../common/Texture.h             \		
        ../common/SphericalCameraManipulator.h   \

#Sources
SOURCES += 	main.cpp			        \
  		../common/Shader.cpp		    \
		../common/Vector.cpp		    \
		../common/Matrix.cpp		    \
		../common/Mesh.cpp		        \
        ../common/Texture.cpp           \
        ../common/SphericalCameraManipulator.cpp \

INCLUDEPATH += 	./ 				    \
		        ../common/ 			\

		
DEFINES += M_PI=3.141592653589793

#Library Libraries
LIBS += ..\lib\freeglutd.lib
LIBS += ..\lib\opengl32.lib
LIBS += ..\lib\glut32.lib
LIBS += ..\lib\glew32.lib
