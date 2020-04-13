/*!
 * \copyright (c) Nokia Corporation and/or its subsidiary(-ies) (qt-info@nokia.com) and/or contributors
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 *
 * \license{This source file is part of QmlOgre abd subject to the BSD license that is bundled
 * with this source code in the file LICENSE.}
 */

#ifndef EXAMPLEAPP_H
#define EXAMPLEAPP_H

#include "ogreengine.h"

#include <QtQuick/QQuickView>

#include "OGRE/RTShaderSystem/OgreRTShaderSystem.h"
#include <QTimer>

class ShaderGeneratorTechniqueResolverListener : public Ogre::MaterialManager::Listener
{
public:

    ShaderGeneratorTechniqueResolverListener(Ogre::RTShader::ShaderGenerator* pShaderGenerator)
    {
        mShaderGenerator = pShaderGenerator;
    }

    /** This is the hook point where shader based technique will be created.
    It will be called whenever the material manager won't find appropriate technique
    that satisfy the target scheme name. If the scheme name is out target RT Shader System
    scheme name we will try to create shader generated technique for it.
    */
    virtual Ogre::Technique* handleSchemeNotFound(unsigned short schemeIndex,
        const Ogre::String& schemeName, Ogre::Material* originalMaterial, unsigned short lodIndex,
        const Ogre::Renderable* rend)
    {
        Ogre::Technique* generatedTech = NULL;

        // Case this is the default shader generator scheme.
        if (schemeName == Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME)
        {
            bool techniqueCreated;

            // Create shader generated technique for this material.
            techniqueCreated = mShaderGenerator->createShaderBasedTechnique(
                originalMaterial->getName(),
                Ogre::MaterialManager::DEFAULT_SCHEME_NAME,
                schemeName);

            // Case technique registration succeeded.
            if (techniqueCreated)
            {
                // Force creating the shaders for the generated technique.
                mShaderGenerator->validateMaterial(schemeName, originalMaterial->getName());

                // Grab the generated technique.
                Ogre::Material::TechniqueIterator itTech = originalMaterial->getTechniqueIterator();

                while (itTech.hasMoreElements())
                {
                    Ogre::Technique* curTech = itTech.getNext();

                    if (curTech->getSchemeName() == schemeName)
                    {
                        generatedTech = curTech;
                        break;
                    }
                }
            }
        }

        return generatedTech;
    }

protected:
    Ogre::RTShader::ShaderGenerator*	mShaderGenerator;			// The shader generator instance.
};



class ExampleApp : public QQuickView
{
    Q_OBJECT
public:
    explicit ExampleApp(QWindow *parent = 0);
    ~ExampleApp();


    void startRTShaderSystem();
    bool initializeRTShaderSystem(Ogre::SceneManager* scene);

    Ogre::RTShader::ShaderGenerator* mShaderGenerator;			// The shader generator instance.
    ShaderGeneratorTechniqueResolverListener *mMaterialMgrListener;

signals:
    void ogreInitialized();

public slots:
    void initializeOgre();
    void addContent();
private:
    OgreEngine *m_ogreEngine;

    Ogre::SceneManager *m_sceneManager;
    Ogre::Root *m_root;
};

#endif // EXAMPLEAPP_H
