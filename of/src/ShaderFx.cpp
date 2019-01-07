

#include "ShaderFx.h"
#ifdef TARGET_RASPBERRY_PI
#include "ofRPIVideoPlayer.h" // for mask shader
#endif



class KaleidoscopeShader : public ShaderBase {
public:

    KaleidoscopeShader(): ShaderBase("kaleidoscope"),
        minAngularVelocity(.010) {
    }

    void initParams()final{
        vAngle = addCustomfP("vAngle", 0);
        debug = addCustomfP("debug", 0);
        _angle  = fParams.getRef("rotation");
        // tstV = leaves.createWithType<Parameter< vector<float> > >("testV",vector<float>({2.f,-2.3f}) );

    };


    void updateParams(float deltaT) final{
        // float distToEnd = abs(fmod(angle+PI,2*PI)-PI);
        // auto vAngle = vAngle.lock();
        auto & vA = vAngle->getValue();//.get();
        auto & angle = *_angle.get();
        if (vA != 0) {
            bool shouldStop = abs(vA) <= minAngularVelocity ;
            // float tst =  abs((float)angle);
            if (!shouldStop || angle != 0) {
                angle += (float)((vA > 0 ? 1 : -1) * MAX(abs(vA), minAngularVelocity) * 2 * PI * deltaT);
                bool hasLooped = false;
                while (angle > 2 * PI) {angle -= 2 * PI; hasLooped = true;}
                while (angle < 0) {angle += 2 * PI; hasLooped = true;}
                if (shouldStop && hasLooped) {angle = 0;}
            }
            else {
                angle = 0;
            }
        }
    }


    FloatParameterListType::ElemPtr vAngle;
    FloatParameterListType::ElemPtr debug;

    float minAngularVelocity;

    FloatParameterListType::ElemPtr  _angle;
    // Parameter<vector<float>>::Ptr tstV;

};

class CurveShader : public ShaderBase {
public:
    const int textureRes = 512; // need to be power of two (shader lut is non ARB)
    CurveShader(): ShaderBase("ShadowHighlights") {
        cTex.allocate(textureRes, 1, GL_RGBA, false);
        const auto texData = cTex.getTextureData();
        pixBuf.allocate(texData.width, texData.height, ofGetImageTypeFromGLType(texData.glInternalFormat));
    }
    void initParams()final{
        low = addCustomvP("lowT", ofVec2f(0.2));
        high = addCustomvP("highT", ofVec2f(0.8));
        lowR = addCustomvP("lowR", ofVec2f(0.2));
        highR = addCustomvP("highR", ofVec2f(0.8));
        lowG = addCustomvP("lowG", ofVec2f(0.2));
        highG = addCustomvP("highG", ofVec2f(0.8));
        lowB = addCustomvP("lowB", ofVec2f(0.2));
        highB = addCustomvP("highB", ofVec2f(0.8));
        populateTexture();
    };
    // bezier

    float interpolatePart(float x, ofVec2f minP, ofVec2f maxP, float minT, float maxT) {
        float delta = maxP.x - minP.x;
        float t = (x - minP.x) / delta;
        float h00 = (1 + 2 * t) * (1 - t) * (1 - t);
        float h10 = t * (1 - t) * (1 - t);
        float h01 = t * t * (3 - 2 * t);
        float h11 = t * t * (t - 1);

        return h00 * minP.y + delta * h10 * minT + h01 * maxP.y + h11 * delta * maxT;
    }
    void populatePixChannel( ofVec2f p1, ofVec2f  p2, int channelNum, vector<ofColor> & cols) {
        p1 *= textureRes;
        p2 *= textureRes;

        float initT = p1.y / p1.x;
        float lastT = (1 - p2.y) / (1 - p2.x);
        float lowT = initT / 2.0 + .5 * (p2.y - p1.y) / (p2.x - p1.x);
        float highT = lowT / 2.0 + lastT / 2.0;


        for (int i = 0 ; i < textureRes  ; i++) {
            float v ;
            if (i < p1.x) {
                v = interpolatePart(i, ofVec2f(0), p1, initT, lowT);
            }
            else if (i < p2.x) {
                v = interpolatePart(i, p1, p2, lowT, highT);
            }
            else {
                v = interpolatePart(i, p2, ofVec2f(1), highT, lastT);
            }
            v *= 255.0 / textureRes;
            float clamped = ofClamp(v, 0, 255);
            cols[i][channelNum] = clamped;

        }


    }
    void populateTexture() {
        vector<ofColor> cols(textureRes, 0);
        populatePixChannel(*low.get(), *high.get(), 3, cols);
        if (defineParams.getRef("MULTI_CHANNEL") && defineParams.getRef("MULTI_CHANNEL")->getValue()) {
            populatePixChannel(*lowR.get(), *highR.get(), 0, cols);
            populatePixChannel(*lowG.get(), *highG.get(), 1, cols);
            populatePixChannel(*lowB.get(), *highB.get(), 2, cols);
        }
        int i = 0;
        for (auto & c : cols) {
            pixBuf.setColor(i, 0, c);
            i++;
        }
        cTex.loadData(pixBuf);

    }
    void parameterValueChanged(ParameterBase * p)final{
        if (customvParams.getRef(p->getName()) != nullptr) {
            populateTexture();
        }

    }

    void updateParams(float deltaT)final{
        shader.setUniformTexture("curveTex", cTex, 1);
    }

    void drawDbg()final{
        cTex.draw(0, 0, textureRes, 10);
    }

    ofTexture cTex;
    Vec2ParameterListType::ElemPtr low, high, lowR, highR, lowG, highG, lowB, highB;

    ofPixels pixBuf;
};


class MaskShader : public ShaderBase {
public:

    MaskShader(): ShaderBase("Mask") {

    }
    void initParams()final{
        maskResolution  = vParams.getRef("maskResolution");
        maskResolution->isSavable = false;
        maskThreshold = fParams.getRef("maskThreshold");
        invertMask = fParams.getRef("invertMask");

        auto setMaskIndex = addParameter<TypedActionParameter<int>>("setMaskIndex", 0, std::bind(&MaskShader::loadMediaAtIdx, this, std::placeholders::_1));
        maskPath = addParameter<StringParameter>("maskPath", "");
        maskPath->setValue("mask1.jpg", this);
    };

    void updateParams(float deltaT)final{
        if (maskPath->hasChanged(true)) { // need to be done while drawing
            loadMediaFromPath(maskPath->getValue());
        }
        if (!maskMedia) {DBGE("no media"); return;}
        if (maskMedia->isLoading()) {DBGW("media is loading"); return;}

        if (maskResolution->getValue() != maskMedia->getSize()) {
            DBGE("oups");
            maskResolution->setValue(maskMedia->getSize());
        }
        if (maskMedia->isAllocated() ) {

            maskMedia->update();
            if (! maskMedia->getTexture()) {
                DBGE("maskImage not unsing texture");
            }
            else {
                auto & tex = *maskMedia->getTexture();
                auto texSize = ofVec2f(tex.getWidth(), tex.getHeight());
                if (!tex.isAllocated()) {
                    DBGE("texture not allocated");
                }
                if ( texSize != maskMedia->getSize()) {
                    DBGE("texture have the wrong size" << texSize);
                }
                shader.setUniformTexture("maskTex", *maskMedia->getTexture(), 1);
            }
        }
        else{
            DBGE("mask not allocated");
        }

    }
    void parameterValueChanged(ParameterBase * p)final{
        if (p == enabled.get()) {
            if (enabled->getValue()) {
                string maskPathWhenDisabled = maskPath->getValue();
                ofLog() << "resetting mask : " << maskPathWhenDisabled;
                maskPath->setValue(""); // to notify reload
                maskPath->setValue(maskPathWhenDisabled);
            }
            else {
                maskMedia = nullptr;
            }
        }
    }
    void loadMediaFromPath(const string & s) {

        string truePath = s;
        if (!ofFilePath::isAbsolute(s) && !ofIsStringInString(s, defaultFolder)) {
            truePath = defaultFolder + s;
        }

        ofFile f(truePath);
        if (f.exists() && f.isFile()) {
            auto ext  = ofFilePath::getFileExt(truePath);
            ofLog() << "opening " << truePath << " : ext : " << ext;
            if (ofContains(VideoMedia::exts, ext)) {
                ofLog() << "opening video";
                maskMedia =nullptr;
                maskMedia = make_shared<VideoMedia>(truePath);
            }
            else if (ofContains(ImageMedia::exts, ext)) {
                ofLog() << "opening image";
                maskMedia =nullptr;
                maskMedia = make_shared<ImageMedia>(truePath);
            }
            else {
                maskMedia = nullptr;
                ofLogError() << "no valid extension for : " << s;
                return;
            }
        }
        if (!maskMedia)                  {DBGE("maskShader :  weird error"); return;}
        if (!maskMedia->isLoading() && !maskMedia->isAllocated())  {DBGE("maskShader :  media not allocated"); return;}

        maskResolution->setValue(maskMedia->getSize());

    };

    void loadMediaAtIdx(const int& idx) {
        ofDirectory mediaFolder("images");

        for (auto & s : ImageMedia::exts) {mediaFolder.allowExt(s);}
        for (auto & s : VideoMedia::exts) {mediaFolder.allowExt(s);}
        int totalNumFile = mediaFolder.listDir();
        auto files = mediaFolder.getFiles();
        ofSort(files);
        if (totalNumFile == 0) {ofLogError() << "no media found" ; return;}
        // if (idx > totalNumFile) {idx %= totalNumFile;}
        maskPath->setValue(files[idx % totalNumFile].getAbsolutePath());


    }

    

    StringParameter::Ptr maskPath;
    FloatParameter::Ptr maskThreshold, invertMask;
    NumericParameter<ofVec2f>::Ptr maskResolution;
    ofFbo maskFbo;
    ofImage maskImg;
    struct AbstractMedia {
        virtual ~AbstractMedia() {};
        virtual ofTexture * getTexture() = 0;
        virtual void update() = 0;
        virtual bool isLoading() = 0;
        virtual ofVec2f getSize() = 0;
        virtual bool isAllocated() = 0;

    };
    struct ImageMedia  : public AbstractMedia {
        static vector<string> exts;
        ImageMedia(const string & path): img(path) {};
        void update()final{};
        bool isLoading()final{return false;}
        ofTexture * getTexture()final{return &img.getTexture();}
        bool isAllocated()final{return img.isAllocated();}
        ofVec2f getSize()final{return {img.getWidth(), img.getHeight()};}
    private:
        ofImage img;
    };

#ifdef TARGET_RASPBERRY_PI
#define VID_PLAYER_TYPE    ofRPIVideoPlayer
#else
#define VID_PLAYER_TYPE ofVideoPlayer
#endif
    struct VideoMedia  : public AbstractMedia {
        static vector<string> exts;
        static ofRPIVideoPlayer & getVidPlayer() {
            static VID_PLAYER_TYPE vp;
            return vp;
        }
        VideoMedia(const string & path) 
        : vid(getVidPlayer()) 
        {
            if (vid.isPlaying()) {
                DBG("video try to close first");
                vid.close();
                int maxWait = 9999999;
                while (vid.isPlaying() && maxWait > 0) {
                    maxWait--;
                }
                if(maxWait==0)DBGE("could'nt wait for vid to stop");
            }
            vid.setLoopState(OF_LOOP_NORMAL);
            vid.load(path);
            // vid.setUseTexture(true);
            // vid.setLoopState(OF_LOOP_NORMAL);
            vid.play();
        };
        ~VideoMedia() {
            DBG("deleting video media ")
            vid.close();
        }
        bool isLoading()final{return false;}
        void update()final{vid.update();};
        ofTexture * getTexture()final{return vid.getTexturePtr();}
        bool isAllocated()final{return vid.isLoaded();}
        ofVec2f getSize()final{return {vid.getWidth(), vid.getHeight()};}
    private:

        VID_PLAYER_TYPE & vid;

    };

    shared_ptr<AbstractMedia> maskMedia;
    static string defaultFolder;

};
string MaskShader::defaultFolder = "images/";
vector<string> MaskShader::ImageMedia::exts {"jpg", "png"};
vector<string> MaskShader::VideoMedia::exts {"mp4"};

class TOONShader: public ShaderBase {
public:
    TOONShader(): ShaderBase("toon") {

    }
    void initParams()final{
        hueRes  = fParams.getRef("hueRes");
        satRes  = fParams.getRef("satRes");
        valRes  = fParams.getRef("valRes");


    };
    void parameterValueChanged(ParameterBase * p)final{
        if (p == hueRes.get() ) {
            if (auto def = defineParams.getRef("USE_HUE")) {
                def->setValue(hueRes->getValue() == 0 ? 0 : 1);
            }
        }
        else if (p == satRes.get() ) {
            if (auto def = defineParams.getRef("USE_SAT")) {
                def->setValue(satRes->getValue() == 0 ? 0 : 1);
            }
        }
        else if (p == valRes.get() ) {
            if (auto def = defineParams.getRef("USE_VAL")) {
                def->setValue(valRes->getValue() == 0 ? 0 : 1);
            }
        }
    };
    FloatParameterListType::ElemPtr hueRes, satRes, valRes;

};

template<class T, class CONTAINER, class ...Args>
void addAndRegisterType (CONTAINER & cont, Args... args) {
    auto inst = new T(args...);
    cont.add(inst);
    ParameterContainerFactory::i().registerType<T>("shader_" + inst->getName(), args...);
    // populate ShaderFx with shaderParameters
    inst->load();
    // inst->unload();
}

void ShaderFx::setup() {

    // addAndRegisterType<ShaderBase>(nodes, "green2gs");
    addAndRegisterType<ShaderBase>(nodes, "blur");
    addAndRegisterType<TOONShader>(nodes);
    addAndRegisterType<ShaderBase>(nodes, "borders");
    addAndRegisterType<ShaderBase>(nodes, "mirror");
    addAndRegisterType<ShaderBase>(nodes, "champi");
    addAndRegisterType<CurveShader>(nodes);
    addAndRegisterType<KaleidoscopeShader>(nodes);
    addAndRegisterType<MaskShader>(nodes);

    // soloShader(availableShaders["blur"]);

}


///////////////////////
//// Utils
////////////////////



const map<string, ShaderBase::DefaultUniform> ShaderBase::reservedUniformsMap {{"resolution", ShaderBase::DefaultUniform::resolution}, {"time", ShaderBase::DefaultUniform::time}, {"mouse", ShaderBase::DefaultUniform::mouse}, {"speed", ShaderBase::DefaultUniform::speed}};

void ShaderBase::autoParseUniforms() {
    if (!isLoaded()) return;
    clearParams();
    ofBuffer fragBuffer = ofBufferFromFile("shaders/" + getName() + ".frag");

    string source = fragBuffer.getText();//shader.getShaderSource(GL_FRAGMENT_SHADER);
    defaultUniformFlags = 0;
    ofStringReplace(source, "\r", "");
    const auto lines = ofSplitString(source, "\n", true, true);
    for (const auto & l : lines) {
        const auto words = ofSplitString(l, " ", true, true);
        if (words.size() >= 3 && (words[0] == "#define" && words[1] != "USE_ARB" && words[1] != "SAMPLER_2D_TYPE" && words[1].find("(") == -1 )) {
            const string pname = words[1];
            addDefineP(pname, ofToInt(words[2]));
        }
        else if (words.size() >= 3) {
            if (words[0] == "uniform" && words[1].find("sampler") == -1) {

                bool isFloat = words[1] == "float";
                bool isVec = words[1] == "vec2";
                bool isVec3 = words[1] == "vec3";
                const string pname = ofSplitString(words[2], ";", true, true)[0];
                const auto & reserved = reservedUniformsMap.find(pname);
                if (reserved != reservedUniformsMap.end()) {
                    setDefaultUniform(reserved->second);
                    continue;
                }
                string defaultV = "";
                if (isFloat || isVec || isVec3) {
                    int commStart = l.find("//");
                    Node::PtrT<ParameterBase> vp(nullptr);
                    if (commStart != -1) {
                        defaultV = l.substr(commStart);
                        int endP = defaultV.find(")");
                        int startP = defaultV.find("(");
                        if (startP != 1 && endP != -1 && endP > startP ) {
                            defaultV = defaultV.substr(startP + 1, endP - startP - 1);
                        }
                        else {
                            defaultV = "";
                        }
                        if (defaultV == "disabled") {
                            continue;
                        }
                    }

                    if (isFloat) {
                        vp = addfP(pname, ofToFloat(defaultV));
                    }
                    else if (isVec) {
                        auto spl = ofSplitString(defaultV, ",");
                        while (spl.size() < 2) {spl.push_back(spl.size() > 0 ? spl[0] : "0");}
                        vp = addvP(pname, ofVec2f(ofToFloat(spl[0]), ofToFloat(spl[1])));
                    }
                    else if (isVec3) {
                        auto spl = ofSplitString(defaultV, ",");
                        while (spl.size() < 3) {spl.push_back(spl.size() > 0 ? spl[0] : "0");}
                        vp = addv3P(pname, ofVec3f(ofToFloat(spl[0]), ofToFloat(spl[1]), ofToFloat(spl[2])));
                    }

                }
            }
        }
    }
}
