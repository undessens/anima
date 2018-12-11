#pragma once

#include "ofMain.h"
#include "ofAppEGLWindow.h"
#include "TerminalListener.h"
#include "ofxOMXVideoGrabber.h"
#include "CameraSettings.h"
#include "ofxOsc.h"

#include "Enhancement.h"
#include "ZoomCrop.h"
#include "Filters.h"
#include "WhiteBalance.h"

#define NB_SETTINGS 4

class ofApp : public ofBaseApp, public KeyListener{
    
public:
    
    void setup();
    void update();
    void draw();
    void keyPressed(int key);
    
    void onCharacterReceived(KeyListenerEventData& e);
    TerminalListener consoleListener;
    
    ofxOMXCameraSettings omxCameraSettings;
    ofxOMXVideoGrabber videoGrabber;
    
    CameraSettings* listOfSettings[NB_SETTINGS];
    ofxOscReceiver receiver;

    string transport;

    bool doDrawInfo;
    bool doPrintInfo;
};
