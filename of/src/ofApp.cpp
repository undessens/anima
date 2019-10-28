#include "AppConfig.h"

#include "ofApp.h"
#include "main.cpp.impl" // weird but remove useless main compilation unit (rpi is slooow)
#include "MediaSource.hpp"

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
    ofSetVerticalSync(true);
#endif
    mediaSource = make_unique<MediaSourcePlayer>();
    webcam = mediaSource->getWebcamSource();
    if(!webcam){
        ofLogError() << "!!!!!!!!!!!!! webcam not initialized !!!!!!!!!!!!!!!" ;
    }
    else{
#ifdef TARGET_RASPBERRY_PI
    root->addSharedParameterContainer(make_shared<OMXController>(webcam->getGrabber()));
#endif
    }
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


    auto tfps = root->addParameter<ActionParameter>("targetFPS", [](const string & s) {float tfps = MAX(10.0, ofToFloat(s)); ofSetFrameRate(tfps); DBG("setting FPS to " << tfps);});
#if DO_STREAM
    auto doStream = root->addParameter<TypedActionParameter<bool> >("doStream", false, [this](const bool & needStream) {getStreamVid().setStreamState(needStream); DBG("setting Streaming to " << (needStream ? "true" : "false"));});
#endif
    displayTestImage = root->addParameter<TypedActionParameter<bool> >("displayTestImage", false, [](const bool & s) {if (s) {getTestImage().load("images/tst.jpg");} else {getTestImage().clear();}});
    auto doDrawInfoParam = root->addParameter<TypedActionParameter<bool> >("displayDebugInfo", false, [this](const bool & s) {doDrawInfo = s;});
    auto reload = root->addParameter<TypedActionParameter<bool> >("reloadShaders", false, [this](const bool & s) { if (!shaderFx->reload()) {ofLogError() << "couldn't reload shader";}});
    auto vSync = root->addParameter<TypedActionParameter<bool> >("Vsync", false, [](const bool & s) { ofSetVerticalSync(s);});
    auto pause = root->addParameter<TypedActionParameter<bool> >("Pause", false, [this](const bool & s) { setAppPaused(s);});
    auto togglePause = root->addParameter<ActionParameter >("togglePause", [this](const string & s) { setAppPaused(!appPaused);});


}
void ofApp::setAppPaused(const bool & s){
    appPaused = s;
//    ofSetBackgroundAuto(!s); // not working on raspberry pi 
    *(shaderFx->bFreeze) = s;

}


//--------------------------------------------------------------
void ofApp::update()
{
    shaderFx->update();
    mediaSource->update();

    processOSC();
}


//--------------------------------------------------------------
void ofApp::draw()
{


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

    ofTexture & drawnTexture = displayTestImage->getValue() ? getTestImage().getTexture() : mediaSource->getTexture();


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

        for (int i = 0; i < webcam->getNumSettings(); i++) {
            // ofLogVerbose() << " For: " << ofToString(i);
            auto setting = webcam->getSetting(i);
            if ( add0 == setting->name) {
                ofLogVerbose() << "\n OSC settings:" << add0 << " - " << add1 << " : " << ofToString(value);

                //ROUTE osc message according the type of settings
                // Then transmit the adress and value to the specific setting class
                setting->onOsc(add1, value);

                //Finally update the class to apply new settings
                setting->update();

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

        case 'u':
        {
            setAppPaused(!appPaused);
            break;
        }
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
        case 'n' :
        {
            *(shaderFx->nodes["toon"]->leaves.getWithTypeNamed<Parameter<bool>>("enabled")) = true;
            *(shaderFx->nodes["kaleidoscope"]->leaves.getWithTypeNamed<Parameter<bool>>("enabled")) = true;
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
            mediaSource->reset();
            break;
        }
        case 'z' :
        {
            mediaSource->startRecording();
            break;
        }
        case 'x' :
        {
            mediaSource->stopRecording();
            break;
        }
#endif
        case 'y' :
        {
            mediaSource->goToNextSource();
            break;
        }
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
        info << "CAMERA RESOLUTION: "   << mediaSource->getWidth() << "x" << mediaSource->getHeight()
#if !EMULATE_ON_OSX
        << " @ " << webcam->getGrabber().getFrameRate() << "FPS"
#endif
        << endl;

        if (doDrawInfo)
        {
            int x = 100;
            if (mediaSource->getWidth() < 1280)
            {
                x = mediaSource->getWidth();
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



