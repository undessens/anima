#pragma once

#include "AppConfig.h"

#include "ofMain.h"
#include "ofxOsc.h"


#if EMULATE_ON_OSX

#else 
// #include "ofAppEGLWindow.h"
#include "TerminalListener.h"
#include "ofxOMXVideoGrabber.h"
#include "CameraSettings.h"
#include "Enhancement.h"
#include "ZoomCrop.h"
#include "Filters.h"
#include "WhiteBalance.h"

#endif



#include "ShaderFx.h"
#include "PresetManager.hpp"

#define NB_SETTINGS 4

class ofApp : public ofBaseApp
#if !EMULATE_ON_OSX
 ,public KeyListener
 #endif
 {
    
public:
    ofApp();
    ~ofApp();
    void setup();
    void update();
    void draw();
    void keyPressed(int key);
    
    #if !EMULATE_ON_OSX
    void onCharacterReceived(KeyListenerEventData& e);
    TerminalListener consoleListener;
    ofxOMXCameraSettings omxCameraSettings;
    ofxOMXVideoGrabber videoGrabber;
    CameraSettings* listOfSettings[NB_SETTINGS];
    #else
    ofVideoGrabber videoGrabber;
    #endif
    
    
    ofxOscReceiver receiver;

    string transport;

    bool doDrawInfo;
    bool doPrintInfo;

    float lastFrameTime;
    shared_ptr<ParameterContainer> root;
    shared_ptr<PresetManager> presetManager;
    shared_ptr<ShaderFx> shaderFx;

    OSCParameterBinder oscBind;


     void drawInfoIfAsked();
     void processOSC();
};
