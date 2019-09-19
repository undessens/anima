#pragma once

#include "AppConfig.h"

#include "ofMain.h"
#include "ofxOsc.h"


#if EMULATE_ON_OSX

#else 
// #include "ofAppEGLWindow.h"
#include "TerminalListener.h"
#include "ofxOMXVideoGrabber.h"
#include "CamEffects/CameraSettings.h"
#include "CamEffects/Enhancement.h"
#include "CamEffects/ZoomCrop.h"
#include "CamEffects/Filters.h"
#include "CamEffects/WhiteBalance.h"

#endif



#include "ShaderFx.h"
#include "ParameterContainer/PresetManager.hpp"

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
    TypedActionParameter<bool>::Ptr displayTestImage;
    shared_ptr<PresetManager> presetManager;
    shared_ptr<ShaderFx> shaderFx;

    OSCParameterBinder oscBind;


     void drawInfoIfAsked();
     void processOSC();

     void initParameters();
     void setAppPaused(const bool & s);
     bool appPaused = false;
};
