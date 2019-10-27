#pragma once

#include "ofMain.h"
//#include "ofAppEGLWindow.h"
#include "ofxOMXVideoGrabber.h"
#include "ImageFilterCollection.h"

class CameraSettings
{
public:
    
    ofxOMXVideoGrabber* videoGrabber;
    ImageFilterCollection filterCollection;
    
    string name;
    string infoString;
    CameraSettings()
    {
        name = "";
        infoString = "";
        filterCollection.setup();
    }
    
    void setup(ofxOMXVideoGrabber* videoGrabber_)
    {
        videoGrabber = videoGrabber_;
    }
    virtual void update()=0;
    virtual void reset()=0;
    virtual void onOsc(string adress, int value)=0;
    
};