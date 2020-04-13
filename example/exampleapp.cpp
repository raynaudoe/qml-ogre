/*!
 * \copyright (c) Nokia Corporation and/or its subsidiary(-ies) (qt-info@nokia.com) and/or contributors
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 *
 * \license{This source file is part of QmlOgre abd subject to the BSD license that is bundled
 * with this source code in the file LICENSE.}
 */

#include "exampleapp.h"

#include "cameranodeobject.h"

#include "ogreitem.h"
#include "ogreengine.h"

#include <QCoreApplication>
#include <QtQml/QQmlContext>
#include <QDir>

static QString appPath()
{
    QString path = QCoreApplication::applicationDirPath();
    QDir dir(path);
#ifdef Q_WS_MAC
    dir.cdUp();
    dir.cdUp();
    dir.cdUp();
#elif defined(Q_WS_WIN)
    dir.cdUp();
#endif
    return dir.absolutePath();
}

ExampleApp::ExampleApp(QWindow *parent) :
    QQuickView(parent)
  , m_ogreEngine(0)
  , m_sceneManager(0)
  , m_root(0)
  , mMaterialMgrListener(NULL)
{
    qmlRegisterType<CameraNodeObject>("Example", 1, 0, "Camera");

    // start Ogre once we are in the rendering thread (Ogre must live in the rendering thread)
    connect(this, &ExampleApp::beforeRendering, this, &ExampleApp::initializeOgre, Qt::DirectConnection);
    connect(this, &ExampleApp::ogreInitialized, this, &ExampleApp::addContent);
}

ExampleApp::~ExampleApp()
{
    if (m_sceneManager) {
        m_root->destroySceneManager(m_sceneManager);
    }
}

void ExampleApp::initializeOgre()
{
    // we only want to initialize once
    disconnect(this, &ExampleApp::beforeRendering, this, &ExampleApp::initializeOgre);

    // start up Ogre
    m_ogreEngine = new OgreEngine(this);
    m_root = m_ogreEngine->startEngine();
    m_ogreEngine->setupResources();

    // set up Ogre scene
    m_sceneManager = m_root->createSceneManager(Ogre::ST_GENERIC, "mySceneManager");
    startRTShaderSystem();

    m_sceneManager->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));
    m_sceneManager->createLight("myLight")->setPosition(20, 80, 50);

    // Resources with textures must be loaded within Ogre's GL context
    m_ogreEngine->activateOgreContext();
    m_sceneManager->getRootSceneNode()->attachObject(m_sceneManager->createEntity("Head", "Sinbad.mesh"));
    m_sceneManager->getRootSceneNode()->scale(35, 35, 1);
    m_ogreEngine->doneOgreContext();

    emit(ogreInitialized());
}

void ExampleApp::addContent()
{
    // expose objects as QML globals
    rootContext()->setContextProperty("Window", this);
    rootContext()->setContextProperty("OgreEngine", m_ogreEngine);

    // load the QML scene
    setResizeMode(QQuickView::SizeRootObjectToView);
    setSource(QUrl("qrc:/qml/example.qml"));
}

bool ExampleApp::initializeRTShaderSystem(Ogre::SceneManager* sceneMgr)
{
    if (Ogre::RTShader::ShaderGenerator::initialize())
    {
        mShaderGenerator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();

        mShaderGenerator->addSceneManager(sceneMgr);

        // Setup core libraries and shader cache path.
        Ogre::StringVector groupVector = Ogre::ResourceGroupManager::getSingleton().getResourceGroups();
        Ogre::StringVector::iterator itGroup = groupVector.begin();
        Ogre::StringVector::iterator itGroupEnd = groupVector.end();
        Ogre::String shaderCoreLibsPath;
        Ogre::String shaderCachePath;

        for (; itGroup != itGroupEnd; ++itGroup)
        {
            Ogre::ResourceGroupManager::LocationList resLocationsList = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList(*itGroup);
            Ogre::ResourceGroupManager::LocationList::iterator it = resLocationsList.begin();
            Ogre::ResourceGroupManager::LocationList::iterator itEnd = resLocationsList.end();
            bool coreLibsFound = false;

            // Try to find the location of the core shader lib functions and use it
            // as shader cache path as well - this will reduce the number of generated files
            // when running from different directories.
            for (; it != itEnd; ++it)
            {
                if ((*it)->archive->getName().find("RTShaderLib") != Ogre::String::npos)
                {
                    shaderCoreLibsPath = (*it)->archive->getName() + "/cache/";
                    shaderCachePath = shaderCoreLibsPath;
                    coreLibsFound = true;
                    break;
                }
            }
            // Core libs path found in the current group.
            if (coreLibsFound)
                break;
        }

        // Core shader libs not found -> shader generating will fail.
        if (shaderCoreLibsPath.empty())
            return false;

        // Create and register the material manager listener if it doesn't exist yet.
        if (mMaterialMgrListener == NULL) {
            mMaterialMgrListener = new ShaderGeneratorTechniqueResolverListener(mShaderGenerator);
            Ogre::MaterialManager::getSingleton().addListener(mMaterialMgrListener);
        }

        // Add a specialized sub-render (per-pixel lighting) state to the default scheme render state
        /*Ogre::RTShader::RenderState* pMainRenderState = mShaderGenerator->createOrRetrieveRenderState(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME).first;
        pMainRenderState->reset();

        mShaderGenerator->addSubRenderStateFactory(new Ogre::RTShader::PerPixelLightingFactory);
        pMainRenderState->addTemplateSubRenderState(
        mShaderGenerator->createSubRenderState(Ogre::RTShader::PerPixelLighting::Type));*/
    } else {
        Q_ASSERT(false);
    }

    return true;
}

void ExampleApp::startRTShaderSystem()
{
    // Initialize shader generator.
    // Must be before resource loading in order to allow parsing extended material attributes.
    bool success = initializeRTShaderSystem(m_sceneManager);
    if (!success)
    {
        OGRE_EXCEPT(Ogre::Exception::ERR_FILE_NOT_FOUND,
                "Shader Generator Initialization failed - Core shader libs path not found",
                "SampleBrowser::createDummyScene");
    }

    return;

    if(m_root->getRenderSystem()->getCapabilities()->hasCapability(Ogre::RSC_FIXED_FUNCTION) == false)
    {
        //newViewport->setMaterialScheme(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);

        // creates shaders for base material BaseWhite using the RTSS
        Ogre::MaterialPtr baseWhite = Ogre::MaterialManager::getSingleton().getByName("BaseWhite", Ogre::ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME);
        if (!baseWhite.get()) {
            qDebug() << "Material basewhite not found!";
        }
        baseWhite->setLightingEnabled(false);
        mShaderGenerator->createShaderBasedTechnique("BaseWhite",
                                  Ogre::MaterialManager::DEFAULT_SCHEME_NAME,
                                  Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
        mShaderGenerator->validateMaterial(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, "BaseWhite");
                if(baseWhite->getNumTechniques() > 1)
                {
            baseWhite->getTechnique(0)->getPass(0)->setVertexProgram(
            baseWhite->getTechnique(1)->getPass(0)->getVertexProgram()->getName());
            baseWhite->getTechnique(0)->getPass(0)->setFragmentProgram(
            baseWhite->getTechnique(1)->getPass(0)->getFragmentProgram()->getName());
                }

        // creates shaders for base material BaseWhiteNoLighting using the RTSS
        Ogre::MaterialPtr baseWhiteNoLighting = Ogre::MaterialManager::getSingleton().getByName("BaseWhiteNoLighting", Ogre::ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME);

        mShaderGenerator->createShaderBasedTechnique("BaseWhiteNoLighting",
                                  Ogre::MaterialManager::DEFAULT_SCHEME_NAME,
                                  Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
        mShaderGenerator->validateMaterial(Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME, "BaseWhiteNoLighting");
                if(baseWhite->getNumTechniques() > 1)
                {
            baseWhiteNoLighting->getTechnique(0)->getPass(0)->setVertexProgram(
            baseWhiteNoLighting->getTechnique(1)->getPass(0)->getVertexProgram()->getName());
            baseWhiteNoLighting->getTechnique(0)->getPass(0)->setFragmentProgram(
            baseWhiteNoLighting->getTechnique(1)->getPass(0)->getFragmentProgram()->getName());
                }
    }
}
