// simple class to test various omx settings

class OMXController : public ParameterContainer{
public:
  OMXController(ofxOMXVideoGrabber & _vidGrab):ParameterContainer("omx"),vidGrab(_vidGrab){

    
    addParameter<TypedActionParameter<float> >("sharpness",0,[this](const float & s){vidGrab.setSharpness(s);});
    addParameter<TypedActionParameter<float> >("contrast",0,[this](const float & s){vidGrab.setContrast(s);});
    addParameter<TypedActionParameter<float> >("brightness",0,[this](const float & s){vidGrab.setBrightness(s);});
    addParameter<TypedActionParameter<float> >("saturation",0,[this](const float & s){vidGrab.setSaturation(s);});
    addParameter<TypedActionParameter<bool> >("frameStabilization",false,[this](const bool & s){vidGrab.setFrameStabilization(s);});
    addParameter<TypedActionParameter<int> >("dre",0,[this](const int & s){vidGrab.setDRE(s);});
    addParameter<TypedActionParameter<bool> >("flickerCancellation",false,[this](const bool & s){vidGrab.setFlickerCancellation(s);});
    addParameter<TypedActionParameter<bool> >("colorDenoise",false,[this](const bool & s){vidGrab.setColorDenoise(s);});
    addParameter<TypedActionParameter<int> >("evCompensation",0,[this](const int & s){vidGrab.setEvCompensation(s);}); // -4 to 4

    addParameter<ActionParameter>("whiteBalance",[this](const string & s){vidGrab.setWhiteBalance(s);});
    addParameter<TypedActionParameter<float> >("whiteBalanceGainR",0,[this](const float & s){vidGrab.setWhiteBalanceGainR(s);});
    addParameter<TypedActionParameter<float> >("whiteBalanceGainB",0,[this](const float & s){vidGrab.setWhiteBalanceGainB(s);});
    addParameter<ActionParameter>("imageFilters",[this](const string & s){vidGrab.setImageFilter(s);});
    
    addParameter<ActionParameter>("exposurePreset",[this](const string & s){vidGrab.setExposurePreset(s);});
    addParameter<ActionParameter>("meteringType",[this](const string & s){vidGrab.setMeteringType(s);});



    addParameter<TypedActionParameter<float> >("shutterSpeed",0,[this](const float & s){vidGrab.setShutterSpeedNormalized(s);});
    addParameter<TypedActionParameter<bool> >("autoShutter",false,[this](const bool & s){vidGrab.setAutoShutter(s);});
    addParameter<TypedActionParameter<bool> >("softwareSharpening",false,[this](const bool & s){vidGrab.setSoftwareSharpening(s);});
    addParameter<TypedActionParameter<bool> >("softwareSaturation",false,[this](const bool & s){vidGrab.setSoftwareSaturation(s);});
    addParameter<TypedActionParameter<bool> >("burstMode",false,[this](const bool & s){vidGrab.setBurstMode(s);});
    
    addParameter<TypedActionParameter<float> >("ISONorm",0,[this](const float & s){vidGrab.setISONormalized(s);});
    addParameter<TypedActionParameter<bool> >("autoISO",false,[this](const bool & s){vidGrab.setAutoISO(s);});
    
  }
  

   ofxOMXVideoGrabber & vidGrab;

};