#pragma once
#include "CameraSettings.h"


class SettingsWhiteBalance  : public CameraSettings
{
    
public:
    
    bool doChangeWhiteBalance;
    size_t currentWhiteBalanceIndex;
    vector<string> whiteBalanceNames;
    void setup(ofxOMXVideoGrabber* videoGrabber_)
    {
        CameraSettings::setup( videoGrabber_);
        doChangeWhiteBalance = false; 
        currentWhiteBalanceIndex = 0;
        whiteBalanceNames = OMX_Maps::getInstance().getWhiteBalanceNames();
        
    };
    
    void update()
    {
      
    }

    void reset()
    {
        currentWhiteBalanceIndex = 0;
        videoGrabber->setWhiteBalance(GetWhiteBalance(whiteBalanceNames[currentWhiteBalanceIndex]));
	
    }

    
    void onOsc(string address, int value)
    {
        if(address == "wbNext" && value > 63)
	{
             if(currentWhiteBalanceIndex+1 < whiteBalanceNames.size())
             {
                 currentWhiteBalanceIndex++;
             }
	     else
             {
                 currentWhiteBalanceIndex = 0;
             }
             videoGrabber->setWhiteBalance(GetWhiteBalance(whiteBalanceNames[currentWhiteBalanceIndex]));
         }
	if(address == "wbPrev" && value > 63 )
	{
             if(currentWhiteBalanceIndex > 0)
             {
                 currentWhiteBalanceIndex--;
             }
	     else
             {
                 currentWhiteBalanceIndex = whiteBalanceNames.size()-1;
             }
             videoGrabber->setWhiteBalance(GetWhiteBalance(whiteBalanceNames[currentWhiteBalanceIndex]));
         }
	if(address == "whiteBalanceGainR" )
	{
             float f = value/127.0 ;
             videoGrabber->setWhiteBalanceGainR(f);
         }
	if(address == "whiteBalanceGainB" )
	{
             float f = value/127.0 ;
             videoGrabber->setWhiteBalanceGainB(f);
         }
	if(address == "shutterSpeed" )
	{
             value = value/127.0 * 500;
             videoGrabber->setShutterSpeed(value);
         }


    }
};
