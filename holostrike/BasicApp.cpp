#include "BasicApp.h"

BasicApp::BasicApp()
    : mShutdown(false),
    mRoot(0),
    mCamera(0),
    mSceneMgr(0),
    mWindow(0),
    mResourcesCfg(Ogre::StringUtil::BLANK),
    mPluginsCfg(Ogre::StringUtil::BLANK),
    mCameraMan(0),
    mRenderer(0),
    mMouse(0),
    mKeyboard(0),
    mInputMgr(0),
    mDistance(0),
    mWalkSpd(70.0),
    mDirection(Ogre::Vector3::ZERO),
    mDestination(Ogre::Vector3::ZERO),
    mAnimationState(0),
    mEntity(0),
    mNode(0)
{
}

BasicApp::~BasicApp()
{
    if (mCameraMan) delete mCameraMan;

    Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
    windowClosed(mWindow);

    delete mRoot;
}

void BasicApp::go()
{
#ifdef _DEBUG
    mResourcesCfg = "resources_d.cfg";
    mPluginsCfg = "plugins_d.cfg";
#else
    mResourcesCfg = "resources.cfg";
    mPluginsCfg = "plugins.cfg";
#endif

    if (!setup())
        return;

    mRoot->startRendering();

    destroyScene();
}

bool BasicApp::frameRenderingQueued(const Ogre::FrameEvent& fe)
{
    if (mKeyboard->isKeyDown(OIS::KC_ESCAPE))
        mShutdown = true;

    if (mShutdown)
        return false;

    if (mWindow->isClosed())
        return false;

    mKeyboard->capture();
    mMouse->capture();

    mCameraMan->frameRendered(fe);

    CEGUI::System::getSingleton().injectTimePulse(fe.timeSinceLastFrame);

    return true;
}

bool BasicApp::keyPressed(const OgreBites::KeyboardEvent& ke)
{
    // CEGUI::GUIContext& context = CEGUI::System::getSingleton().getDefaultGUIContext();
    // context.injectKeyDown((CEGUI::Key::Scan)ke.key);
    // context.injectChar((CEGUI::Key::Scan)ke.text);

    mCameraMan->keyPressed(ke);

    return true;
}

bool BasicApp::keyReleased(const OgreBites::KeyboardEvent& ke)
{
    // CEGUI::GUIContext& context = CEGUI::System::getSingleton().getDefaultGUIContext();
    // context.injectKeyUp((CEGUI::Key::Scan)ke.key);

    mCameraMan->keyReleased(ke);

    return true;
}

bool BasicApp::mouseMoved(const OgreBites::MouseMotionEvent& me)
{
    // CEGUI::GUIContext& context = CEGUI::System::getSingleton().getDefaultGUIContext();
    // context.injectMouseMove(me.state.X.rel, me.state.Y.rel);

    mCameraMan->mouseMoved(me);

    return true;
}

// Helper function for mouse events
CEGUI::MouseButton convertButton(OIS::MouseButtonID id)
{
    switch (id)
    {
    case OIS::MB_Left:
        return CEGUI::LeftButton;
    case OIS::MB_Right:
        return CEGUI::RightButton;
    case OIS::MB_Middle:
        return CEGUI::MiddleButton;
    default:
        return CEGUI::LeftButton;
    }
}

bool BasicApp::mousePressed(const OgreBites::MouseButtonEvent& me, OIS::MouseButtonID id)
{
    // CEGUI::GUIContext& context = CEGUI::System::getSingleton().getDefaultGUIContext();
    // context.injectMouseButtonDown(convertButton(id));

    mCameraMan->mousePressed(me);

    return true;
}

bool BasicApp::mouseReleased(const OgreBites::MouseButtonEvent& me, OIS::MouseButtonID id)
{
    // CEGUI::GUIContext& context = CEGUI::System::getSingleton().getDefaultGUIContext();
    // context.injectMouseButtonUp(convertButton(id));

    mCameraMan->mouseReleased(me);

    return true;
}

void BasicApp::windowResized(Ogre::RenderWindow* rw)
{
    unsigned int width, height, depth;
    int left, top;
    rw->getMetrics(width, height, left, top);

    const OIS::MouseState& ms = mMouse->getMouseState();
    ms.width = width;
    ms.height = height;
}

void BasicApp::windowClosed(Ogre::RenderWindow* rw)
{
    if (rw == mWindow)
    {
        if (mInputMgr)
        {
            mInputMgr->destroyInputObject(mMouse);
            mInputMgr->destroyInputObject(mKeyboard);

            OIS::InputManager::destroyInputSystem(mInputMgr);
            mInputMgr = 0;
        }
    }
}

bool BasicApp::setup()
{
    mRoot = new Ogre::Root(mPluginsCfg);

    setupResources();

    if (!configure())
        return false;

    chooseSceneManager();
    createCamera();
    createViewports();

    Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

    createResourceListener();
    loadResources();

    setupCEGUI();

    createScene();

    createFrameListener();

    return true;
}

bool BasicApp::configure()
{
    if (!(mRoot->restoreConfig() || mRoot->showConfigDialog()))
    {
        return false;
    }

    mWindow = mRoot->initialise(true, "ITutorial");

    return true;
}

void BasicApp::chooseSceneManager()
{
    int sceneType = 2;
    mSceneMgr = mRoot->createSceneManager();
}

void BasicApp::createCamera()
{
    mCamera = mSceneMgr->createCamera("PlayerCam");

    // create a SceneNode to attach the camera to
    mNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("PlayerCamNode");
    mNode->attachObject(mCamera);

    // set the position of the SceneNode
    mNode->setPosition(Ogre::Vector3(0, 0, 80));

    // get the direction from the camera position to the target position
    Ogre::Vector3 target = Ogre::Vector3(0, 0, -300);
    Ogre::Vector3 direction = target - mNode->getPosition();
    direction.normalise();

    // set the direction the camera is looking
    mNode->setDirection(direction);

    // set the near clip distance
    mCamera->setNearClipDistance(5);

    // create a camera controller
    mCameraMan = new OgreBites::CameraMan(mCamera);
}

void BasicApp::createScene()
{
    // set lights
    mSceneMgr->setAmbientLight(Ogre::ColourValue(1.0, 1.0, 1.0));

    // create roobot
    mEntity = mSceneMgr->createEntity("robot.mesh");

    mNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(
        Ogre::Vector3(0, 0, 25.0));
    mNode->attachObject(mEntity);
    // create walk path for robot
    mWalkList.push_back(Ogre::Vector3(550.0, 0, 50.0));
    mWalkList.push_back(Ogre::Vector3(-100.0, 0, -200.0));
    mWalkList.push_back(Ogre::Vector3(0, 0, 25.0));
    // add other objects into scene
    Ogre::Entity* ent;
    Ogre::SceneNode* node;
    Ogre::SceneNode* camNode;

    ent = mSceneMgr->createEntity("knot.mesh");
    node = mSceneMgr->getRootSceneNode()->createChildSceneNode(
        Ogre::Vector3(0, -10.0, 25.0));
    node->attachObject(ent);
    node->setScale(0.1, 0.1, 0.1);

    ent = mSceneMgr->createEntity("knot.mesh");
    node = mSceneMgr->getRootSceneNode()->createChildSceneNode(
        Ogre::Vector3(550.0, -10.0, 50.0));
    node->attachObject(ent);
    node->setScale(0.1, 0.1, 0.1);

    ent = mSceneMgr->createEntity("knot.mesh");
    node = mSceneMgr->getRootSceneNode()->createChildSceneNode(
        Ogre::Vector3(-100.0, -10.0, -200.0));
    node->attachObject(ent);
    node->setScale(0.1, 0.1, 0.1);
    // create a camera
    camNode = mNode->getChild("PlayerCamNode");
    camNode->setPosition(Ogre::Vector3(90.0, 280.0, 535.0));
    camNode->pitch(Ogre::Degree(-30.0));
    camNode->yaw(Ogre::Degree(-15.0));

    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();
    CEGUI::Window* rootWin = wmgr.loadLayoutFromFile("test.layout");

    CEGUI::System::getSingleton().getDefaultGUIContext().setRootWindow(rootWin);

}

void BasicApp::destroyScene()
{
}

void BasicApp::createFrameListener()
{
    Ogre::LogManager::getSingletonPtr()->logMessage("*** Initializing OIS ***");

    OIS::ParamList pl;
    size_t windowHnd = 0;
    std::ostringstream windowHndStr;

    mWindow->getCustomAttribute("WINDOW", &windowHnd);
    windowHndStr << windowHnd;
    pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

    mInputMgr = OIS::InputManager::createInputSystem(pl);

    mKeyboard = static_cast<OIS::Keyboard*>(
        mInputMgr->createInputObject(OIS::OISKeyboard, true));
    mMouse = static_cast<OIS::Mouse*>(
        mInputMgr->createInputObject(OIS::OISMouse, true));

    mKeyboard->setEventCallback(this);
    mMouse->setEventCallback(this);

    windowResized(mWindow);

    Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

    mRoot->addFrameListener(this);

    Ogre::LogManager::getSingletonPtr()->logMessage("Finished");
}

void BasicApp::createViewports()
{
    Ogre::Viewport* vp = mWindow->addViewport(mCamera);
    vp->setBackgroundColour(Ogre::ColourValue(0, 0, 0));

    mCamera->setAspectRatio(
        Ogre::Real(vp->getActualWidth()) /
        Ogre::Real(vp->getActualHeight()));
}

void BasicApp::setupResources()
{
    Ogre::ConfigFile cf;
    cf.load(mResourcesCfg);

    Ogre::String secName, typeName, archName;
    Ogre::ConfigFile::SectionIterator secIt = cf.getSectionIterator();

    while (secIt.hasMoreElements())
    {
        secName = secIt.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap* settings = secIt.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator setIt;

        for (setIt = settings->begin(); setIt != settings->end(); ++setIt)
        {
            typeName = setIt->first;
            archName = setIt->second;
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                archName, typeName, secName);
        }
    }
}

void BasicApp::createResourceListener()
{
}

void BasicApp::loadResources()
{
    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

bool BasicApp::setupCEGUI()
{
    Ogre::LogManager::getSingletonPtr()->logMessage("*** Initializing CEGUI ***");

    mRenderer = &CEGUI::OgreRenderer::bootstrapSystem();

    CEGUI::ImageManager::setImagesetDefaultResourceGroup("Imagesets");
    CEGUI::Font::setDefaultResourceGroup("Fonts");
    CEGUI::Scheme::setDefaultResourceGroup("Schemes");
    CEGUI::WidgetLookManager::setDefaultResourceGroup("LookNFeel");
    CEGUI::WindowManager::setDefaultResourceGroup("Layouts");

    CEGUI::SchemeManager::getSingleton().createFromFile("TaharezLook.scheme");
    CEGUI::FontManager::getSingleton().createFromFile("DejaVuSans-10.font");

    CEGUI::GUIContext& context = CEGUI::System::getSingleton().getDefaultGUIContext();

    context.setDefaultFont("DejaVuSans-10");
    context.getMouseCursor().setDefaultImage("TaharezLook/MouseArrow");

    Ogre::LogManager::getSingletonPtr()->logMessage("Finished");

    return true;
}


#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT)
#else
    int main(int argc, char* argv[])
#endif
    {
        BasicApp app;

        try
        {
            app.go();
        }
        catch (Ogre::Exception& e)
        {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            MessageBox(
                NULL,
                e.getFullDescription().c_str(),
                "An exception has occured!",
                MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
            std::cerr << "An exception has occured: " <<
                e.getFullDescription().c_str() << std::endl;
#endif
        }

        return 0;
    }

#ifdef __cplusplus
}
#endif