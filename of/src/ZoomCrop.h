#pragma once
#include "CameraSettings.h"


class SettingsZoomCrop  : public CameraSettings
{
    
public:
    
    
    int marginWidth;
    int marginHeight;
    int zoomValue;
    void setup(ofxOMXVideoGrabber* videoGrabber_)
    {
        CameraSettings::setup( videoGrabber_);
	marginWidth = 0;
        marginHeight= 0;
        zoomValue = 100;

       
    }
    
    void update()
    {

            //videoGrabber->zoomIn();
            //videoGrabber->zoomOut();
	    //videoGrabber->setSensorCrop(0, 0,  randomPercentage, randomPercentage);
    }
    
    void reset()
    {
        videoGrabber->setZoomLevelNormalized(0);
    }
    
    void onOsc(string address, int value)
    {      
        if (address == "zoomLevel")
        {
            	zoomValue = 100 - (value/127.0) * 100;
		marginWidth = (value/127.0) * 50 + 1;
		marginHeight = (value/127.0) * 50 + 1;
		videoGrabber->setSensorCrop(marginWidth, marginHeight,  zoomValue, zoomValue);
	}

	if(address == "leftMargin")
	{
  		marginWidth = (value/127.0) * 98 + 1;
		videoGrabber->setSensorCrop(marginWidth, marginHeight,  zoomValue, zoomValue);
	}

		if(address == "topMargin")
	{
  		marginHeight = (value/127.0) * 98 + 1;
		videoGrabber->setSensorCrop(marginWidth, marginHeight,  zoomValue, zoomValue);
	}

    }

};
