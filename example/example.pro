CONFIG += qt
CONFIG += warn_off
QT += qml quick
TEMPLATE = app
TARGET = qmlogre

LIBS +=


linux-arm-imx6* {
    message("compiling for iMX6")
    INCLUDEPATH += /mnt/boundary_etnaviv/usr/include/OGRE/ \
                   /mnt/boundary_etnaviv/usr/include/OGRE/RTShaderSystem \
                   /mnt/boundary_etnaviv/usr/include/OGRE/RenderSystems/GLES2/ \

    DEFINES += JIBO_GLES2
    LIBS+=-L/mnt/boundary_etnaviv/usr/lib/ -lOgreRTShaderSystem -lOgreMain
}

#linux-arm-imx6* {
#    message("compiling for iMX6-etnaviv")
#    INCLUDEPATH += /mnt/boundary_etnaviv/usr/include/OGRE/ \
#                   /mnt/boundary_etnaviv/usr/include/OGRE/RTShaderSystem \
#                   /mnt/boundary_etnaviv/usr/include/OGRE/RenderSystems/GL \

#    DEFINES += JIBO_GL
#    LIBS+=-L/mnt/boundary_etnaviv/usr/lib/ -lOgreRTShaderSystem -lOgreMain
#}


#linux-* {
#    message("compiling for PC")
#    INCLUDEPATH += /usr/include/OGRE \
#                   /usr/include/OGRE/RTShaderSystem \
#                   /mnt/rebel/usr/include/OGRE/RenderSystems/GL \

#    DEFINES += JIBO_GL
#    LIBS+=-L/usr/lib -lOgreRTShaderSystem -lOgreMain
#}


UI_DIR = ./.ui
OBJECTS_DIR = ./.obj
MOC_DIR = ./.moc
SOURCES += main.cpp \
    cameranodeobject.cpp \
    exampleapp.cpp \
    ogreitem.cpp \
    ogrenode.cpp \
    ogrecamerawrapper.cpp \
    ogreengine.cpp


HEADERS += cameranodeobject.h \
    exampleapp.h \
    ogreitem.h \
    ogrenode.h \
    ogrecamerawrapper.h \
    ogreengine.h

OTHER_FILES += \
    resources/example.qml




RESOURCES += resources/resources.qrc

# Copy all resources to build folder
Resources.path = $$OUT_PWD/resources
Resources.files = resources/*.zip

# Copy all config files to build folder
Config.path = $$OUT_PWD
Config.files = config/*

# make install
INSTALLS += Resources Config

DISTFILES += \
    plugins.cfg \
    resource.cfg
