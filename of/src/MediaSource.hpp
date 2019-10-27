//inline bool ends_with(std::string const & value, std::string const & ending)
//{
//    if (ending.size() > value.size()) return false;
//    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
//}
#define FREE_FILESOURCE_WHEN_NOT_USED 1

class MediaSource{
public:
    MediaSource(const string & _uri):uri(_uri){}
    virtual ~MediaSource(){
        isDeleting=true;
        free();
    }
    virtual string getName() {return uri;}


    void load(bool force = false,bool autoPlay=true){
        if(!isLoaded || force){
            isLoaded=innerLoad();
        };
        if(autoPlay){
        innerPlay();

        }

    }

    void play(){
        if(isLoaded){
            innerPlay();}
        else{
            ofLogError() << "can't play, media not loaded" ;
        }
    }

    void stop(){
        if(isLoaded){
            innerPlay();}
        else{
            ofLogError() << "can't stop, media not loaded" ;
        }
    }
    void free(bool force = false){
        innerStop();
        if( !freeOnlyWhenDeleted || isDeleting || force){
            innerFree();
            isLoaded=false;
        }
    }
    void update(){
        if(isLoaded){
            innerUpdate();
        }
    }
    ofTexture & getTexture(){
        //        if(isLoaded){
        //        return &innerGetTexture();
        //
        //    }
        //        return nullptr;
        return innerGetTexture();

    }
    virtual ofBaseDraws & getBaseDraw() = 0;

    const string & getURI(){return uri;}
protected:
    virtual void innerFree(){};
    virtual bool innerLoad() = 0;
    virtual void innerUpdate() {};
    virtual void innerStop() {};
    virtual void innerPlay() {};

    virtual ofTexture & innerGetTexture() = 0;

    const string uri;
    bool freeOnlyWhenDeleted = false;
    bool isLoaded = false;
    bool isDeleting = false;

};


class WebcamSource : public MediaSource{
public:

    static string numDeviceToURI(const int i){return "webcam:"+ofToString(i);}

    WebcamSource(int num):
    MediaSource(WebcamSource::numDeviceToURI(num))
    {
        freeOnlyWhenDeleted = true;
    }
    bool innerLoad() override{
#if !EMULATE_ON_OSX
        ofxOMXCameraSettings settings;
        ofBuffer jsonBuffer = ofBufferFromFile("settings.json");
        settings.parseJSON(jsonBuffer.getText());
        settings.enableTexture = bool(USE_SHADERS);

        videoGrabber.setup(settings);

        
        // ofSetFrameRate(30);
        int settingsCount = 0;

        SettingsEnhancement* enhancement = new SettingsEnhancement();
        enhancement->setup(&videoGrabber);
        enhancement->name = "enhancement";
        listOfSettings[settingsCount] = enhancement;
        settingsCount++;

        SettingsZoomCrop* zoomCrop = new SettingsZoomCrop();
        zoomCrop->setup(&videoGrabber);
        zoomCrop->name = "zoomCrop";
        listOfSettings[settingsCount] = zoomCrop;
        settingsCount++;

        SettingsFilters* filters = new SettingsFilters();
        filters->setup(&videoGrabber);
        filters->name = "filters";
        listOfSettings[settingsCount] = filters;
        settingsCount++;

        SettingsWhiteBalance* whiteBalance = new SettingsWhiteBalance();
        whiteBalance->setup(&videoGrabber);
        whiteBalance->name = "whiteBalance";
        listOfSettings[settingsCount] = whiteBalance;
        settingsCount++;
#else
        videoGrabber.setup(ofGetWidth(), ofGetHeight(), true);
#endif
        return true;
    }

    void innerUpdate() override{
        #if EMULATE_ON_OSX
        //        if (!displayTestImage->getValue()) {
        videoGrabber.update();
        //        }
        #endif
        // OMX dont have update method
    }

    ofTexture & innerGetTexture() override{
        return videoGrabber.
#if EMULATE_ON_OSX
        getTexture();
#else
        getTextureReference();
#endif
    }
    ofBaseDraws & getBaseDraw() override{
        return innerGetTexture();
    }
    #if !EMULATE_ON_OSX
    ofxOMXVideoGrabber & getGrabber(){return videoGrabber;}
    CameraSettings* getSetting(const int i){return listOfSettings[i];}
    int getNumSettings(){return NB_SETTINGS;}
    #endif
private:
#if !EMULATE_ON_OSX
    ofxOMXCameraSettings omxCameraSettings;
    ofxOMXVideoGrabber videoGrabber;
    int NB_SETTINGS = 4;
    CameraSettings* listOfSettings[4];

#else
    ofVideoGrabber videoGrabber;
#endif
};

class FileSource : public MediaSource{
public:


    static ofDirectory defaultFolder;

    FileSource(const string & uri):MediaSource(uri),mediaFile(uri){

        freeOnlyWhenDeleted = false; // could keep media cached
    }
    virtual ~FileSource(){

    }

    static string normalizeURI(const string & uri){
        if(uri.length()>0){
            if(uri[0]!='/'){
                return FileSource::defaultFolder.getAbsolutePath()+uri;
            }
        }
        return uri;
    }
    ofFile mediaFile;
};

class ImageFileSource  :public FileSource{
public:
    static vector<string> & getValidExtensions() {
        static vector<string> exts= {"png","jpg","jpeg","bmp"};
        return exts;
    }

    ImageFileSource(const string & uri):FileSource(uri){}

    ofTexture & innerGetTexture()override{ return img.getTexture();}
    bool innerLoad()override{return img.load(mediaFile.getAbsolutePath());}
    ofBaseDraws & getBaseDraw() override{return img;}
private:
    ofImage img;

};

class VideoFileSource : public FileSource{
public:
    static vector<string> & getValidExtensions() {
        static vector<string> exts= {"avi","mov","mp4"};
        return exts;
    }

    VideoFileSource(const string & uri):FileSource(uri){}
    void innerUpdate() override{vid.update();}
    void innerFree() override {vid.close();}
    ofTexture & innerGetTexture() override{ return vid.getTexture();}
    bool innerLoad() override {return vid.load(mediaFile.getAbsolutePath());}
    void innerStop() override {vid.stop();}
    void innerPlay() override {vid.play();}
    ofBaseDraws & getBaseDraw() override{return vid;}
private:
    ofVideoPlayer vid;
};



class MediaSourcePlayer{
public:

    typedef  enum{
        NONE,
        WEBCAM ,
        FILE
    }SOURCE_TYPE;

    MediaSourcePlayer(){
#if EMULATE_ON_OSX

        FileSource::defaultFolder  = ofDirectory("/Users/Tintamar/Documents/phone_bup/WhatsApp/Media/WhatsApp Video");
#else
        FileSource::defaultFolder  = ofDirectory("medias");
#endif
        setupSources();

    }

    static bool isValidExtension(const string & u, const vector<string> & allowed ){
        for(auto & e:allowed){
            if (u==e){
                return true;
            }
        }
        return false;

    }
    void goToNextSource(){
        if(medias.size()){
            setSourceFromURI(medias[(currentSourceIdx+1)%medias.size()]->getURI());
        }
    }

    void setSourceFromURI(const string uri){ // 0 is camera, >0 is files located in default folder
        ArrayPtr<MediaSource> newSource = nullptr;
        int i = 0;
        int newSourceIdx = -1;
        for(auto & m:medias){
            if(m->getURI()==uri){
                newSource = m.get();
                newSourceIdx = i;
                break;
            }
            i++;
        }

        if(newSource){
            if(auto lastS = getCurrentSource()){
                lastS->free();
            }
            newSource->load();
        }
        currentSourceIdx = newSourceIdx;
        //        currentSource = newSource; //Idx already set before
    }

    void update(){if(auto cs = getCurrentSource()){cs->update();}}
    //    void draw(){if(currentSource){currentSource->draw();}}
    ofTexture &  getTexture(){if(auto cs = getCurrentSource()){return cs->getTexture();} return fakeTexture;}
    int getWidth(){if(auto cs = getCurrentSource()){return cs->getBaseDraw().getWidth();}return -1;}
    int getHeight(){if(auto cs = getCurrentSource()){return cs->getBaseDraw().getHeight();}return -1;}

    // TODO
    void startRecording(){}
    void stopRecording(){}
    void reset(){};
    MediaSource * getCurrentSource(){
        return currentSourceIdx>=0?medias[currentSourceIdx].get():nullptr;
    }

    WebcamSource *getWebcamSource(){return webcamGrabberRef;}

private:

    void setupSources(){
        freeAll();
        medias.emplace_back(new WebcamSource(0));
        webcamGrabberRef = dynamic_cast<WebcamSource*>(medias[0].get());
        setSourceFromURI(WebcamSource::numDeviceToURI(0));
        auto parsedFolder = FileSource::defaultFolder;
        if(parsedFolder.exists()){
            parsedFolder.listDir();
            parsedFolder.sort();
            for(auto & f:parsedFolder.getFiles()){
                if(MediaSource* s = createFileSource(f)){
                    medias.emplace_back(s);
                }
            }

        }
        else{
            ofLogError() << "no folder found at " << parsedFolder.getAbsolutePath();
        }
    }
    MediaSource*  createFileSource(const ofFile & f){
        if(f.isFile()){
            auto ext = ofToLower(f.getExtension());
            auto uri = f.getAbsolutePath();
            if(isValidExtension(ext,VideoFileSource::getValidExtensions())){
                return new VideoFileSource(uri);
            }
            else if(isValidExtension(ext,ImageFileSource::getValidExtensions())){
                return new ImageFileSource(uri);
            }
        }
        return nullptr;
    }

    void freeAll(){
        currentSourceIdx =-1;
        for( auto & w:medias){w->free();}
        medias.clear();

    }

    template<class T>
    using OwnedArray  = vector<unique_ptr<T>>;

    template<class T>
    using ArrayPtr = T *;

    OwnedArray<MediaSource> medias;

    int  currentSourceIdx = -1;
    ofTexture fakeTexture;
    WebcamSource * webcamGrabberRef=nullptr;
};


///////////
// implementation


ofDirectory FileSource::defaultFolder = ofDirectory();
