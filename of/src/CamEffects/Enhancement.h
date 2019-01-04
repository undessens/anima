#pragma once
#include "CameraSettings.h"


class SettingsEnhancement  : public CameraSettings
{
    
public:
    
    

    
    int sharpness;
    int contrast;
    int brightness;
    int saturation;
    

    
    void setup(ofxOMXVideoGrabber* videoGrabber_)
    {
        CameraSettings::setup( videoGrabber_);        
        //videoGrabber->enableAutoExposure();
    }
    
    void update()
    {
    }

    void reset()

    {
            videoGrabber->setSharpness(-50);
            videoGrabber->setContrast(-10);
            videoGrabber->setBrightness(50);
            videoGrabber->setSaturation(0);       
    }
        

    
    void onOsc(string address, int value)
    {
        if (address == "contrast")
        {
             int newContrast = int(ofMap(value, 0, 127, -100, 100));
	     videoGrabber->setContrast(newContrast);
	     ofLogVerbose() << "\ngrabber contrast changed : " << ofToString(newContrast);
        }
        if (address == "sharpness")
        {
             int newSharp = int(ofMap(value, 0, 127, -100, 100));
	     videoGrabber->setSharpness(newSharp);
	     ofLogVerbose() << "\ngrabber sharpness changed : " << ofToString(newSharp);
        }
        if (address == "saturation")
        {
             int newSat = int(ofMap(value, 0, 127, -100, 100));
	     videoGrabber->setSaturation(newSat);
	     ofLogVerbose() << "\ngrabber saturation changed : " << ofToString(newSat);
        }
        if (address == "brightness")
        {
             int newBri = int(ofMap(value, 0, 127, 0, 100));
         videoGrabber->setBrightness(newBri);
         ofLogVerbose() << "\ngrabber brightness changed : " << ofToString(newBri);
        }

  
    }
};
