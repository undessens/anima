#include "AppConfig.h"

#include "ofApp.h"
#include "main.cpp.impl" // weird but remove useless main compilation unit (rpi is slooow)


#define CAN_MAP_VIDEO 1
#if CAN_MAP_VIDEO
#include "ofxGLWarper.h"
ofxGLWarper warper;
ParameterContainer::Ptr warperParameters;
NumericParameter<ofVec2f>::Ptr topLeft,topRight, bottomLeft, bottomRight;
ParameterContainer::LambdaListener warperSync("warperSync",[](ParameterContainer * parent,ParameterBase * changedP){
    ofVec2f screenSize (ofGetWidth(),ofGetHeight());
    glm::vec2 tl( topLeft->getValue()*screenSize);
    glm::vec2 tr( topRight->getValue()*screenSize);
    glm::vec2 bl( bottomLeft->getValue()*screenSize);
    glm::vec2 br( bottomRight->getValue()*screenSize);
    warper.setAllCorners(tl,tr,bl,br);
    warper.save();

});
#endif




#define USE_SHADERS 1

#ifndef DO_STREAM
#define DO_STREAM 0
#endif

#if DO_STREAM
#include "StreamVid.h"
StreamVid & getStreamVid() {static StreamVid streamVid; return streamVid;}
#endif

ofImage & getTestImage() {static ofImage testImg; return testImg;}

#ifdef TARGET_RASPBERRY_PI
#include "OMXController.hpp"
#endif


ofApp::ofApp() {}
ofApp::~ofApp() {}
//--------------------------------------------------------------
void ofApp::setup()
{
    // shaderFx = new ShaderFx();
    ofSetLogLevel(OF_LOG_VERBOSE);
    ofSetLogLevel("ofThread", OF_LOG_ERROR);
    ofSetLogLevel("ofShader", OF_LOG_WARNING);
    doDrawInfo = false;
    doPrintInfo = false;


    //Hide mouse
    ofHideCursor();

    // OSC
    receiver.setup(12345);

    // Shaders
    root = make_shared<ParameterContainer>("root");
    Node::setRoot(root);
    presetManager = make_shared<PresetManager>(root);
    root->addSharedParameterContainer(presetManager);
    presetManager->setup(ofFile("presets").getAbsolutePath());

#ifdef TARGET_RASPBERRY_PI
    root->addSharedParameterContainer(make_shared<OMXController>(videoGrabber));
#endif

    shaderFx = make_shared<ShaderFx>();
    root->addSharedParameterContainer(shaderFx);
#if USE_SHADERS
    shaderFx->setup();
#endif
    
#if CAN_MAP_VIDEO

    warper.setup();

    warper.load();
    warper.activate(false);
    // if no file found
    if( warper.getCorner(ofxGLWarper::CornerLocation::TOP_RIGHT)==glm::vec2(0,0)){
        glm::vec2 v;
        v= glm::vec2(ofGetWidth(),0);
        warper.setCorner(ofxGLWarper::CornerLocation::TOP_RIGHT,    v);
        v =glm::vec2(0,ofGetHeight());
        warper.setCorner(ofxGLWarper::CornerLocation::BOTTOM_LEFT,  v);
        v =  glm::vec2(ofGetWidth(),ofGetHeight());
        warper.setCorner(ofxGLWarper::CornerLocation::BOTTOM_RIGHT,v);
    }
    warperParameters = make_shared<ParameterContainer>("mapping");

    topLeft     = warperParameters->
    addParameter<NumericParameter<ofVec2f>>("tl",
                                            warper.getCorner(ofxGLWarper::CornerLocation::TOP_LEFT)/ glm::vec2(ofGetWidth(),ofGetHeight()));

    topRight    = warperParameters->
    addParameter<NumericParameter<ofVec2f>>("tr",
                                            warper.getCorner(ofxGLWarper::CornerLocation::TOP_RIGHT)/ glm::vec2(ofGetWidth(),ofGetHeight()));


    bottomLeft  = warperParameters->
    addParameter<NumericParameter<ofVec2f>>("bl",
                                            warper.getCorner(ofxGLWarper::CornerLocation::BOTTOM_LEFT)/ glm::vec2(ofGetWidth(),ofGetHeight()));

    bottomRight = warperParameters->
    addParameter<NumericParameter<ofVec2f>>("br",
                                            warper.getCorner(ofxGLWarper::CornerLocation::BOTTOM_RIGHT)/ glm::vec2(ofGetWidth(),ofGetHeight()));

    warperParameters->parameterContainerListeners.add(&warperSync);
    root->addSharedParameterContainer(warperParameters);
#endif
    oscBind.setup(root, "localhost", 11001);


    lastFrameTime = ofGetElapsedTimef();

#if !EMULATE_ON_OSX
    //allows keys to be entered via terminal remotely (ssh)
    consoleListener.setup(this);

    ofxOMXCameraSettings settings;
    ofBuffer jsonBuffer = ofBufferFromFile("settings.json");
    settings.parseJSON(jsonBuffer.getText());
    settings.enableTexture = bool(USE_SHADERS);

    videoGrabber.setup(settings);

    ofSetVerticalSync(true);
    // ofSetFrameRate(30);
    int settingsCount = 0;

    SettingsEnhancement* enhancement = new SettingsEnhancement();
    enhancement->setup(&videoGrabber);
    enhancement->name = "enhancement";
    listOfSettings[settingsCount] = enhancement;
    settingsCount++;

    SettingsZoomCrop* zoomCrop = new SettingsZoomCrop();
    zoomCrop->setup(&videoGrabber);
    zoomCrop->name = "zoomCrop";
    listOfSettings[settingsCount] = zoomCrop;
    settingsCount++;

    SettingsFilters* filters = new SettingsFilters();
    filters->setup(&videoGrabber);
    filters->name = "filters";
    listOfSettings[settingsCount] = filters;
    settingsCount++;

    SettingsWhiteBalance* whiteBalance = new SettingsWhiteBalance();
    whiteBalance->setup(&videoGrabber);
    whiteBalance->name = "whiteBalance";
    listOfSettings[settingsCount] = whiteBalance;
    settingsCount++;
#else
    videoGrabber.setup(ofGetWidth(), ofGetHeight(), true);
#endif

    initParameters();
    for (auto  s : shaderFx->availableShaders.getNamedPtrSet().vIterator()) {
        presetManager->recallPreset(s, "1");
        s->enabled->setValue(false); 
        // s->enabled->setValue(s->getName() == "ShadowHighlights"); // keep only the curves on
    }


#if USE_ARB
    ofEnableArbTex();
#else
    ofDisableArbTex();
#endif


#if DO_STREAM
    getStreamVid().setup();
#endif






}

void ofApp::initParameters() {


    auto tfps = root->addParameter<ActionParameter>("targetFPS", [this](const string & s) {float tfps = MAX(10.0, ofToFloat(s)); ofSetFrameRate(tfps); DBG("setting FPS to " << tfps);});
#if DO_STREAM
    auto doStream = root->addParameter<TypedActionParameter<bool> >("doStream", false, [this](const bool & needStream) {getStreamVid().setStreamState(needStream); DBG("setting Streaming to " << (needStream ? "true" : "false"));});
#endif
    displayTestImage = root->addParameter<TypedActionParameter<bool> >("displayTestImage", false, [this](const bool & s) {if (s) {getTestImage().load("images/tst.jpg");} else {getTestImage().clear();}});
    auto doDrawInfoParam = root->addParameter<TypedActionParameter<bool> >("displayDebugInfo", false, [this](const bool & s) {doDrawInfo = s;});
    auto reload = root->addParameter<TypedActionParameter<bool> >("reloadShaders", false, [this](const bool & s) { if (!shaderFx->reload()) {ofLogError() << "couldn't reload shader";}});
    auto vSync = root->addParameter<TypedActionParameter<bool> >("Vsync", false, [this](const bool & s) { ofSetVerticalSync(s);});
    auto pause = root->addParameter<TypedActionParameter<bool> >("Pause", false, [this](const bool & s) { setAppPaused(s);});


}
void ofApp::setAppPaused(const bool & s){
    appPaused = s;
    ofSetBackgroundAuto(!s);

}


//--------------------------------------------------------------
void ofApp::update()
{
    shaderFx->update();
#if EMULATE_ON_OSX
    if (!displayTestImage->getValue()) {
        videoGrabber.update();
    }
#endif

    processOSC();
}


//--------------------------------------------------------------
void ofApp::draw()
{
    if(appPaused){return;}

#if CAN_MAP_VIDEO
    ofFill();
    ofSetColor(ofColor::black);
    ofDrawRectangle(0,0,ofGetWidth(),ofGetHeight());
    ofSetColor(ofColor::white);
    warper.begin();
#endif
    if (displayTestImage->getValue()) {
        getTestImage().resize(ofGetWidth(), ofGetHeight());
    }

    ofTexture & drawnTexture = displayTestImage->getValue() ? getTestImage().getTexture() :
#if EMULATE_ON_OSX
                               videoGrabber.getTexture();
#else
                               videoGrabber.getTextureReference();
#endif

#if USE_SHADERS
    ofSetColor(255);
    auto curT = ofGetElapsedTimef();
    float deltaT = curT - lastFrameTime ;
    lastFrameTime  = curT;

    shaderFx->draw(drawnTexture, deltaT);
    shaderFx->drawDbg();

#elif EMULATE_ON_OSX
    drawnTexture.draw(0, 0, ofGetWidth(), ofGetHeight());
#endif

#if CAN_MAP_VIDEO
    warper.end();
#endif

    drawInfoIfAsked();
#if DO_STREAM
    getStreamVid().publishScreen(); // has internal control over fps
#endif
    ofSetColor(255);


}

void ofApp::processOSC() {
    while (receiver.hasWaitingMessages()) {
        // get the next message
        ofxOscMessage m;
        receiver.getNextMessage(m);


        int value = m.getArgAsInt32(0);
        auto splitted = ofSplitString(m.getAddress(), "/", true);

        if ( oscBind.processOSC(m, 0)) continue;

        if (splitted.size() < 2)continue;

        string add0 = splitted[0];
        string add1 = splitted[1];
        // ofLogVerbose() << "\n osc message received add0: " << add0 << " add1 " << add1 << " value: " << ofToString(m.getArgAsString(0));
        ofLogVerbose() << "\n osc message received: " << m;

#if !EMULATE_ON_OSX
        for (int i = 0; i < NB_SETTINGS; i++) {
            // ofLogVerbose() << " For: " << ofToString(i);

            if ( add0 == (listOfSettings[i]->name)) {
                ofLogVerbose() << "\n OSC settings:" << add0 << " - " << add1 << " : " << ofToString(value);

                //ROUTE osc message according the type of settings
                // Then transmit the adress and value to the specific setting class
                (listOfSettings[i])->onOsc(add1, value);

                //Finally update the class to apply new settings
                (listOfSettings[i])->update();

            }


        }
        // ofLogVerbose() << " end of For: " ;
#endif

        if ( add0 == "transport") {
            transport = add1;
        }



    }
}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key)
{
    ofLog(OF_LOG_VERBOSE, "%c keyPressed", key);
    switch (key)
    {

    case 'd':
    {
        doDrawInfo = !doDrawInfo;
        break;
    }
    case 'i':
    {
        doPrintInfo = !doPrintInfo;
        break;
    }
    case 'p' :
    {
        presetManager->savePreset(shaderFx, "1");
        break;
    }
    case 'm' :
    {
        presetManager->recallPreset(shaderFx, "1");
        break;
    }
    case 's': {
        if (!shaderFx->reload()) {
            ofLogError() << "couldn't reload shader";
        }
        break;
    }
#if CAN_MAP_VIDEO
        case 'w':{
            warper.toggleActive();
            break;
        }
        case '+':{
            int selectedCorner = warper.getSelectedCornerLocation();
            if(selectedCorner<0) selectedCorner = 0;
            else{selectedCorner++;selectedCorner%=4;}
//            static_cast<ofxGLWarper::CornerLocation>
            warper.selectCorner((ofxGLWarper::CornerLocation)selectedCorner);
            break;
        }
            #endif


#if !EMULATE_ON_OSX
    case 'r' :
    {
        videoGrabber.reset();
        break;
    }
    case 'z' :
    {
        videoGrabber.startRecording();
        break;
    }
    case 'x' :
    {
        videoGrabber.stopRecording();
        break;
    }
#endif
    default:
    {
        break;
    }

    }
}

#if !EMULATE_ON_OSX
void ofApp::onCharacterReceived(KeyListenerEventData& e)
{
    keyPressed((int)e.character);
}
#endif

void ofApp::drawInfoIfAsked() {
    // if (ofGetFrameNum() % 60 * 2 == 0) {ofLogVerbose() << ofGetFrameRate();}

    if (doDrawInfo || doPrintInfo)
    {
        stringstream info;
        info << endl;
        info << "App FPS: " << ofGetFrameRate() << endl;
        info << "CAMERA RESOLUTION: "   << videoGrabber.getWidth() << "x" << videoGrabber.getHeight()
#if !EMULATE_ON_OSX
             << " @ " << videoGrabber.getFrameRate() << "FPS"
#endif
             << endl;

        if (doDrawInfo)
        {
            int x = 100;
            if (videoGrabber.getWidth() < 1280)
            {
                x = videoGrabber.getWidth();
            }
            ofDrawBitmapStringHighlight(info.str(), x, 40, ofColor(ofColor::black, 50), ofColor::yellow);
        }
        if (doPrintInfo)
        {
            info << shaderFx->getInfo() << endl;
            ofLogVerbose() << info.str();
            doPrintInfo = false;
        }
    }
}



