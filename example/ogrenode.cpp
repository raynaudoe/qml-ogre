#ifdef JIBO_GL
#include <RenderSystems/GL/OgreGLFBORenderTexture.h>
#include <RenderSystems/GL/OgreGLFrameBufferObject.h>
#include <RenderSystems/GL/OgreGLTexture.h>
typedef Ogre::GLFrameBufferObject OgreFrameBufferObject;
typedef Ogre::GLTexture OgreTexture;
typedef Ogre::GLFBOManager OgreFBOManager;
#endif

#ifdef JIBO_GLES2
#include <RenderSystems/GLES2/OgreGLES2FBORenderTexture.h>
#include <RenderSystems/GLES2/OgreGLES2FrameBufferObject.h>
#include <RenderSystems/GLES2/OgreGLES2Texture.h>
typedef Ogre::GLES2FrameBufferObject OgreFrameBufferObject;
typedef Ogre::GLES2Texture OgreTexture;
typedef Ogre::GLES2FBOManager OgreFBOManager;
#endif

#include "ogrenode.h"

#include <Ogre.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QThread>

OgreNode::OgreNode()
    : QSGGeometryNode()
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
    , m_texture(0)
    , m_ogreEngineItem(0)
    , m_camera(0)
    , m_renderTarget(0)
    , m_viewport(0)
    , m_window(0)
    , m_ogreFboId(0)
    , m_dirtyFBO(false)
{
    setMaterial(&m_material);
    setOpaqueMaterial(&m_materialO);
    setGeometry(&m_geometry);
    //setFlag(UsePreprocess);

//    timer.setInterval(4000);
//    timer.setSingleShot(false);
//    QObject::connect(&timer, &QTimer::timeout, [this](){updateFBO();});
//    timer.start();
}

OgreNode::~OgreNode()
{
    if (m_renderTarget) {
        m_renderTarget->removeAllViewports();
    }

    if (Ogre::Root::getSingletonPtr()) {
        Ogre::Root::getSingletonPtr()->detachRenderTarget(m_renderTarget);
    }
}

void OgreNode::setOgreEngineItem(OgreEngine *ogreRootItem)
{
    m_ogreEngineItem = ogreRootItem;
}

void OgreNode::doneOgreContext()
{
    qDebug() << "m_ogreFboId" << m_ogreFboId;
    m_ogreEngineItem->doneOgreContext();
}

void OgreNode::activateOgreContext()
{
    m_ogreEngineItem->activateOgreContext();
    m_ogreEngineItem->ogreContext()->functions()->glBindFramebuffer(GL_FRAMEBUFFER, m_ogreFboId);
}

GLuint OgreNode::getOgreFboId()
{
    if (!m_renderTarget) {
        qDebug() << __PRETTY_FUNCTION__ <<"ERROR";
        return -1;
    }

    OgreFrameBufferObject *ogreFbo = 0;
    m_renderTarget->getCustomAttribute("FBO", &ogreFbo);
    OgreFBOManager *manager = ogreFbo->getManager();
    manager->bind(m_renderTarget);

    GLint id;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &id);

    return id;
}

void OgreNode::preprocess()
{
    qDebug() << __PRETTY_FUNCTION__<< "START" << QThread::currentThread();
    activateOgreContext();
    m_renderTarget->update(true);
    doneOgreContext();
    qDebug() << __PRETTY_FUNCTION__ << "END";
}

void OgreNode::createDummyTexture()
{
    int samples = m_ogreEngineItem->ogreContext()->format().samples();
    Ogre::TexturePtr dummy_tex = Ogre::TextureManager::getSingleton().createManual("DummyTex",
                                                                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                                    Ogre::TEX_TYPE_2D,
                                                                    m_size.width(),
                                                                    m_size.height(),
                                                                    0,
                                                                    Ogre::PF_R8G8B8A8,
                                                                    Ogre::TU_RENDERTARGET, 0, false,
                                                                    samples);

     Ogre::RenderTexture* texture = dummy_tex->getBuffer()->getRenderTarget();

     OgreFrameBufferObject *ogreFbo = 0;
     texture->getCustomAttribute("FBO", &ogreFbo);
     OgreFBOManager *manager = ogreFbo->getManager();
     manager->bind(texture);
     GLint id;
     glGetIntegerv(GL_FRAMEBUFFER_BINDING, &id);
     qDebug() << __PRETTY_FUNCTION__ << id;

    if (texture) {
        Ogre::TextureManager::getSingleton().remove("DummyTex");
        //delete texture;
    }

}

void OgreNode::update()
{
    qDebug() << __PRETTY_FUNCTION__ << "START";
    if (m_dirtyFBO) {
        activateOgreContext();
        updateFBO();
        m_ogreFboId = getOgreFboId();
        m_dirtyFBO = false;
        doneOgreContext();
    }

    activateOgreContext();
    m_renderTarget->update(true);
    doneOgreContext();

    qDebug() << m_renderTarget->getAverageFPS();
}

void OgreNode::updateFBO()
{
    qDebug() << __PRETTY_FUNCTION__ << "START";
    if (m_renderTarget) {
        Ogre::TextureManager::getSingleton().remove("RttTex");
    }

    int samples = m_ogreEngineItem->ogreContext()->format().samples();
    m_rttTexture = Ogre::TextureManager::getSingleton().createManual("RttTex",
                                                                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                                    Ogre::TEX_TYPE_2D,
                                                                    m_size.width(),
                                                                    m_size.height(),
                                                                    0,
                                                                    Ogre::PF_R8G8B8,
                                                                    Ogre::TU_RENDERTARGET, 0, false,
                                                                    samples);

    m_renderTarget = m_rttTexture->getBuffer()->getRenderTarget();

    m_renderTarget->addViewport(m_camera);
    m_renderTarget->getViewport(0)->setClearEveryFrame(true);
    m_renderTarget->getViewport(0)->setBackgroundColour(Ogre::ColourValue::Red);
    m_renderTarget->getViewport(0)->setOverlaysEnabled(false);

    Ogre::Real aspectRatio = Ogre::Real(m_size.width()) / Ogre::Real(m_size.height());
    m_camera->setAspectRatio(aspectRatio);

    QSGGeometry::updateTexturedRectGeometry(&m_geometry,
                                            QRectF(0, 0, m_size.width(), m_size.height()),
                                            QRectF(0, 0, 1, 1));

    OgreTexture *nativeTexture = static_cast<OgreTexture *>(m_rttTexture.get());

    delete m_texture;
    m_texture = m_ogreEngineItem->createTextureFromId(nativeTexture->getGLID(), m_size);

    m_material.setTexture(m_texture);
    m_materialO.setTexture(m_texture);
    qDebug() << __PRETTY_FUNCTION__ << "END";
}

void OgreNode::setSize(const QSize &size)
{
    if (size == m_size)
        return;

    m_size = size;
    m_dirtyFBO = true;
    markDirty(DirtyGeometry);
}
