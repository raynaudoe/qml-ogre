/*!
 * \copyright (c) Nokia Corporation and/or its subsidiary(-ies) (qt-info@nokia.com) and/or contributors
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 *
 * \license{This source file is part of QmlOgre abd subject to the BSD license that is bundled
 * with this source code in the file LICENSE.}
 */

#include "ogreengine.h"
#include "ogreitem.h"

#include <QOpenGLFunctions>
#include <QScreen>
#include <QThread>
#include <QQuickWindow>

OgreEngine::OgreEngine(QQuickWindow *window)
    : QObject(),
      m_resources_cfg(Ogre::StringUtil::BLANK)
{
    qmlRegisterType<OgreItem>("Ogre", 1, 0, "OgreItem");
    qmlRegisterType<OgreEngine>("OgreEngine", 1, 0, "OgreEngine");

    setQuickWindow(window);
}

OgreEngine::~OgreEngine()
{
    delete m_ogreContext;
}

Ogre::Root* OgreEngine::startEngine()
{
    qDebug() << __PRETTY_FUNCTION__ << "START";
    m_resources_cfg = "/usr/share/OGRE/resources.cfg";

    activateOgreContext();

    Ogre::Root *ogreRoot = new Ogre::Root("/usr/share/OGRE/plugins.cfg", "ogre.cfg", "ogre.log");
    Ogre::RenderSystemList list = ogreRoot->getAvailableRenderers();
    ogreRoot->setRenderSystem(list[0]);
    ogreRoot->initialise(false);

    Ogre::NameValuePairList params;

    params["externalGLControl"] = "true";
    params["currentGLContext"] = "true";

    //Finally create our window.
    m_ogreWindow = ogreRoot->createRenderWindow("OgreWindow", 1, 1, false, &params);
    m_ogreWindow->setVisible(false);
    m_ogreWindow->update(false);

    doneOgreContext();

    qDebug() << __PRETTY_FUNCTION__ << "END";
    return ogreRoot;
}

void OgreEngine::stopEngine(Ogre::Root *ogreRoot)
{
    if (ogreRoot) {
//        m_root->detachRenderTarget(m_renderTexture);
        // TODO tell node(s) to detach

    }

    delete ogreRoot;
}

void OgreEngine::setQuickWindow(QQuickWindow *window)
{
    Q_ASSERT(window);

    m_quickWindow = window;
    m_qtContext = QOpenGLContext::currentContext();
    QSurfaceFormat format = m_quickWindow->requestedFormat();
    // create a new shared OpenGL context to be used exclusively by Ogre
    m_ogreContext = new QOpenGLContext();
    m_ogreContext->setFormat(m_quickWindow->requestedFormat());
    m_ogreContext->setShareContext(m_qtContext);
    m_ogreContext->create();
}

void OgreEngine::activateOgreContext()
{
    m_qtContext->functions()->glUseProgram(0);
    m_qtContext->doneCurrent();

    m_ogreContext->makeCurrent(m_quickWindow);
}

void OgreEngine::doneOgreContext()
{
    m_ogreContext->functions()->glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_ogreContext->functions()->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    m_ogreContext->functions()->glBindRenderbuffer(GL_RENDERBUFFER, 0);
    m_ogreContext->functions()->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_ogreContext->doneCurrent();

    m_qtContext->makeCurrent(m_quickWindow);
}

QOpenGLContext* OgreEngine::ogreContext() const
{
    return m_ogreContext;
}

QSGTexture* OgreEngine::createTextureFromId(uint id, const QSize &size, QQuickWindow::CreateTextureOptions options) const
{
    return m_quickWindow->createTextureFromId(id, size, options);
}

void OgreEngine::setupResources(void)
{
    // Load resource paths from config file
    Ogre::ConfigFile cf;
    cf.load(m_resources_cfg);

    // Go through all sections & settings in the file
    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

    Ogre::String secName, typeName, archName;
    while (seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            archName = i->second;

            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                archName, typeName, secName);
        }
    }

    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}
