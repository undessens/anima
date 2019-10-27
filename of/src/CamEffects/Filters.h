 #pragma once
#include "CameraSettings.h"


class SettingsFilters  : public CameraSettings
{
    
public:
    
    
    vector<string> listOfFilter;
    int currentFilter;
    void setup(ofxOMXVideoGrabber* videoGrabber_)
    {
        CameraSettings::setup( videoGrabber_);
        listOfFilter.push_back("None"); // None is the filter number 0, as defined in reset mode
        listOfFilter.push_back("Negative");
        listOfFilter.push_back("Sketch");
        listOfFilter.push_back("WashedOut");
        listOfFilter.push_back("Cartoon");
        listOfFilter.push_back("Hatch");
        listOfFilter.push_back("Posterise");
        listOfFilter.push_back("ColourSwap");
        currentFilter = 0;         
               
    }
    
    void update()
    {

    }

    void reset()
    {
        videoGrabber->setImageFilter("None");
        currentFilter = 0;
    }
    
    
    void onOsc(string address, int value)
    {
        if (address == "initFilter" && value > 63)
        {

                    currentFilter = 0;
		    string name = "";
                    name = listOfFilter[currentFilter];
                    videoGrabber->setImageFilter(name);
		    ofLogVerbose() << "\nInitFilter ";

            
        }

	if (address == "filterNum")
        {

                if(value>0 && value<listOfFilter.size())
                {
                    string name = "";
                    name = listOfFilter[0];
                    videoGrabber->setImageFilter(name);
                    currentFilter = value;
                }
            
        }

        if ( address== "nextFilter" && value > 63)
        {
            currentFilter++;
            if(currentFilter >= listOfFilter.size()) currentFilter = 0;
            string name = listOfFilter[currentFilter];
            videoGrabber->setImageFilter(name);
            ofLogVerbose() << "\nNextFilter " << name;

        }


        if ( address== "previousFilter" && value > 63)
        {
            currentFilter--;
            if(currentFilter < 0) currentFilter = ( listOfFilter.size() - 1 );
            string name = listOfFilter[currentFilter];
            videoGrabber->setImageFilter(name);
	    ofLogVerbose() << "\n PrevFilter " << name;

        }
        
    }
};
