// simple class to test various omx settings


#define HAVE_PHOTO 0

class OMXController : public ParameterContainer,private ofxOMXVideoGrabberListener
#if HAVE_PHOTO
,private ofxOMXPhotoGrabberListener 
#endif
{
public:
    OMXController(ofxOMXVideoGrabber & _vidGrab): ParameterContainer("omx"), vidGrab(_vidGrab) {
        
        vidGrab.settings.recordingFilePath = ofFile("images/0_anima.mp4").getAbsolutePath();
        ofLog() << "setting recording path to : " << vidGrab.settings.recordingFilePath;
        vidGrab.settings.videoGrabberListener = this;
        


        addParameter<TypedActionParameter<float> >("sharpness", 0, [this](const float & s) {vidGrab.setSharpness(s);});
        addParameter<TypedActionParameter<float> >("contrast", 0,    [this](const float & s) {vidGrab.setContrast(s);});
        addParameter<TypedActionParameter<float> > ("brightness", 0,  [this](const float & s) {vidGrab.setBrightness(s);});
        addParameter<TypedActionParameter<float> >("saturation", 0,  [this](const float & s) {vidGrab.setSaturation(s);});
        addParameter<TypedActionParameter<bool> >("frameStabilization", false, [this](const bool & s) {vidGrab.setFrameStabilization(s);});
        addParameter<TypedActionParameter<int> >("dre", 0, [this](const int & s) {vidGrab.setDRE(s);});
        addParameter<TypedActionParameter<bool> >("flickerCancellation", false, [this](const bool & s) {vidGrab.setFlickerCancellation(s);});
        addParameter<TypedActionParameter<bool> >("colorDenoise", false, [this](const bool & s) {vidGrab.setColorDenoise(s);});
        addParameter<TypedActionParameter<int> >("evCompensation", 0, [this](const int & s) {vidGrab.setEvCompensation(s);}); // -4 to 4

        addParameter<ActionParameter>("whiteBalance", [this](const string & s) {vidGrab.setWhiteBalance(s);});
        addParameter<TypedActionParameter<float> >("whiteBalanceGainR", 0, [this](const float & s) {vidGrab.setWhiteBalanceGainR(s);});
        addParameter<TypedActionParameter<float> >("whiteBalanceGainB", 0, [this](const float & s) {vidGrab.setWhiteBalanceGainB(s);});
        addParameter<ActionParameter>("imageFilters", [this](const string & s) {vidGrab.setImageFilter(s);});

        addParameter<ActionParameter>("exposurePreset", [this](const string & s) {vidGrab.setExposurePreset(s);});
        addParameter<ActionParameter>("meteringType", [this](const string & s) {vidGrab.setMeteringType(s);});



        addParameter<TypedActionParameter<float> >("shutterSpeed", 0, [this](const float & s) {vidGrab.setShutterSpeedNormalized(s);});
        addParameter<TypedActionParameter<bool> >("autoShutter", false, [this](const bool & s) {vidGrab.setAutoShutter(s);});
        addParameter<TypedActionParameter<bool> >("softwareSharpening", false, [this](const bool & s) {vidGrab.setSoftwareSharpening(s);});
        addParameter<TypedActionParameter<bool> >("softwareSaturation", false, [this](const bool & s) {vidGrab.setSoftwareSaturation(s);});
        addParameter<TypedActionParameter<bool> >("burstMode", false, [this](const bool & s) {vidGrab.setBurstMode(s);});

        addParameter<TypedActionParameter<float> >("ISONorm", 0, [this](const float & s) {vidGrab.setISONormalized(s);});
        addParameter<TypedActionParameter<bool> >("autoISO", false, [this](const bool & s) {vidGrab.setAutoISO(s);});
        addParameter<TypedActionParameter<float> >("zoom", 0, [this](const float & s) {vidGrab.setDigitalZoomFromFloat(s);});
        addParameter<TypedActionParameter<ofVec2f> >("enhancement", ofVec2f(0.0), [this](const ofVec2f & s) {vidGrab.setColorEnhancement(s.x > 0 || s.y > 0 , s.x, s.y);}); // -4 to 4

        

      
        
        
        
        auto colorsP = addParameter<TypedActionParameter<ofVec3f> >("colors", ofVec3f(255.0), [this](const ofVec3f & s) {
            
            // if(disableWBP->getValue()){
                auto ns = s.getNormalized() * 255.0;
                vidGrab.extraImageFXController.setImageFilter(OMX_ImageFilterColourBalance, {0, (int)ns.x, (int)ns.y, (int)ns.z, 0, 0});
            // }

        }); 

        addParameter<TypedActionParameter<bool> >("disableWhiteB", false, [this,colorsP](const bool & s) {
            vidGrab.setWhiteBalance(s ? "None" : "Auto"); vidGrab.setWhiteBalanceGainR(1.0); vidGrab.setWhiteBalanceGainB(1.0);
            static ofVec3f tempColorsValue(255);
            // #if 
            if (s) {
                // vidGrab.extraImageFXController.setImageFilter(OMX_ImageFilterColourBalance, {0, (int)midColorWB.x, (int)midColorWB.y, (int)midColorWB.z, 0, 0});
                colorsP->setValue(tempColorsValue);
            }
            else {
                tempColorsValue = colorsP->getValue();
                colorsP->setValue(ofVec3f(255.0));
                // vidGrab.extraImageFXController.setImageFilter(OMX_ImageFilterColourBalance, {0, 255, 255, 255, 0, 0});
            }
            // #endif
        });



        vidGrab.extraImageFXController.setImageFilter(OMX_ImageFilterColourBalance, {0, 255, 255, 255, 0, 0});
        //     filterParamConfig.addParam("lens", 0, 255, 0);
        // filterParamConfig.addParam("r", 0, 255, 255);
        // filterParamConfig.addParam("g", 0, 255, 255);
        // filterParamConfig.addParam("b", 0, 255, 255);
        // filterParamConfig.addParam("u", 0, 255, 0);
        // filterParamConfig.addParam("v", 0, 255, 0);

        addParameter<TypedActionParameter<bool> >("rec", false, [this](const bool & s) {startStopRec(s);});

        #if HAVE_PHOTO
        // vidGrab.settings.photoGrabberListener = this;
        photoGrab.setup(vidGrab.settings,this);
        addParameter<TypedActionParameter<bool> >("snapshot", false, [this](const bool & s) {if(s){photoGrab.takePhoto(1);}});
        #endif
    }

    void startStopRec(bool s){
        if (s > 0) {
                vidGrab.settings.recordingFilePath = ofFile("images/0_anima.mp4").getAbsolutePath();
                ofLog() << "setting recording path to : " << vidGrab.settings.recordingFilePath;
                vidGrab.settings.videoGrabberListener = this;
                ofLog() << string("start recording to : ") << vidGrab.settings.recordingFilePath;
                ofAddListener(ofEvents().update, this, &OMXController::onUpdate);
                vidGrab.startRecording();
            }
            else {
                ofLog() << "stop recording";
                ofRemoveListener(ofEvents().update, this, &OMXController::onUpdate);
                vidGrab.stopRecording();
            }
    }
    void onUpdate(ofEventArgs & args) {
        if(vidGrab.isRecording()){
            if(vidGrab.getRecordedFrameCounter()>30*25){
                DBGE("stop recording (more than 30 sec)");
                startStopRec(false);
            }
        }

    }

    void onRecordingComplete(string filePath)final{
        ofLog() << " video recorded at " << filePath;
    };
    ofxOMXVideoGrabber & vidGrab;

#if HAVE_PHOTO
     void onTakePhotoComplete(string filePath)final{
        ofLog() << " photo saved at " << filePath;
     };
     void onPhotoGrabberEngineStart() final{};
    ofxOMXPhotoGrabber photoGrab;
#endif
    
    

};