#include "AppConfig.h"

#include "ShaderFx.h"




class KaleidoscopeShader : public ShaderBase {
public:

    KaleidoscopeShader(): ShaderBase("kaleidoscope"),
    minAngularVelocity(.10) {
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
    const int textureRes = 512; // need to be power of two (shader don't use ARB)
    CurveShader(): ShaderBase("ShadowHighlights") {
        cTex.allocate(textureRes, 1, GL_RGBA, false);
        cTex.readToPixels(pixBuf);
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
    void populatePixChannel( ofVec2f p1,ofVec2f  p2,int channelNum,vector<ofColor> & cols) {
        p1*=textureRes;
        p2*=textureRes;

        // DBG("populating texture : " << low->getValue() << ":::" << high->getValue());

        float initT = p1.y / p1.x;
        float lastT = (1 - p2.y) / (1 - p2.x);
        float lowT = initT / 2.0 + .5 * (p2.y - p1.y) / (p2.x - p1.x);
        float highT = lowT / 2.0 + lastT / 2.0;
        // ostringstream os;

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
//            auto col = ofColor(clamped);
//            if (v < 0) {col[1] = 255;}
//            if (v > 255) {col[2] = 0;}
            // os <<(int)v << ", ";
            cols[i][channelNum] = clamped;
            // pixBuf.setColor(i,1,col);
        }
        // DBG(os.str());

    }
    void populateTexture(){
        vector<ofColor> cols(textureRes,0);
        populatePixChannel(*low.get(),*high.get(),3,cols);
        populatePixChannel(*lowR.get(),*highR.get(),0,cols);
        populatePixChannel(*lowG.get(),*highG.get(),1,cols);
        populatePixChannel(*lowB.get(),*highB.get(),2,cols);
        int i = 0;
        for(auto & c:cols){
            pixBuf.setColor(i,0,c);
            i++;
        }
        cTex.loadData(pixBuf);

    }
    void parameterValueChanged(ParameterBase * p)final{
        if(customvParams.getRef(p->getName())!=nullptr){
            populateTexture();
        }

    }

    void updateParams(float deltaT)final{
//        if (low->hasChanged(true) || high->hasChanged(true)) {
//            populateTexture();
//        }
        shader.setUniformTexture("curveTex", cTex, 1);
    }

    void drawDbg()final{
        cTex.draw(0, 0, textureRes, 10);
    }

    ofTexture cTex;
    Vec2ParameterListType::ElemPtr low,high,lowR,highR,lowG,highG,lowB,highB;

    ofPixels pixBuf;
};


class MaskShader : public ShaderBase{
public:
    MaskShader():ShaderBase("Mask"){

    }
    void initParams()final{
        maskResolution  = vParams.getRef("maskResolution");
        maskResolution->isSavable= false;
        maskThreshold = fParams.getRef("maskThreshold");
        invertMask = fParams.getRef("invertMask");
        setMaskPath = addParameter<ActionParameter>("setMaskPath",std::bind(&MaskShader::loadImage,this,std::placeholders::_1));
        maskPath = addParameter<StringParameter>("maskPath","");
        maskPath->setValue("tst.jpg",this);

    };
    void updateParams(float deltaT)final{
        if(maskPath->hasChanged(true)) {// need to be don whi
            loadImage(maskPath->getValue());
        }
        if(maskResolution->getValue()!=ofVec2f(maskImg.getWidth(),maskImg.getHeight())){
            DBGE("oups");

        }
        if(maskImg.isAllocated()){
            if(!maskImg.isUsingTexture()){
                DBGE("maskImage not unsing texture");
            }
            shader.setUniformTexture("maskTex", maskImg.getTexture(), 1);
        }
//        shader.setUniformTexture("maskTex", maskFbo.getTexture(), 1);
    }
    void parameterValueChanged(ParameterBase * p)final{
        //        if(p==)
    }
    void loadImage(const string & s){
        {   ofFile f(s);
            if(f.exists()){ofImage im(f);setImage(im);return;}
        }
        {
            ofFile f(string("images/")+s);
            if(f.exists()){
                ofImage im(f);
                setImage(im);
                return;
            }
        }


        DBGE("image not found " << s);

    };

    void setImage( ofImage & im ){
        if(!im.isAllocated()){
            DBGE("maskShader :  image not allocated");
            return;
        }
////        auto fboType = im.getImageType()==OF_IMAGE_GRAYSCALE?GL_LUMINANCE:GL_RGBA;
//        ofFbo::Settings settings(nullptr);
//        settings.width = im.getWidth();
//        settings.height = im.getHeight();
////        int		numColorbuffers;		// how many color buffers to create
////        std::vector<GLint> colorFormats;		// format of the color attachments for MRT.
//        settings.useDepth = false;				// whether to use depth buffer or not
//        settings.useStencil= false;				// whether to use stencil buffer or not
//        settings.depthStencilAsTexture = false;			// use a texture instead of a renderbuffer for depth (useful to draw it or use it in a shader later)
//        settings.textureTarget=GL_TEXTURE_RECTANGLE_ARB;			// GL_TEXTURE_2D or GL_TEXTURE_RECTANGLE_ARB
//        settings.internalformat =GL_RGBA;			// GL_RGBA, GL_RGBA16F_ARB, GL_RGBA32F_ARB, GL_LUMINANCE32F_ARB etc.
////        GLint	depthStencilInternalFormat; 	// GL_DEPTH_COMPONENT(16/24/32)
////        int		wrapModeHorizontal;		// GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER etc.
////        int		wrapModeVertical;		// GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER etc.
//        settings.minFilter = GL_LINEAR;				// GL_NEAREST, GL_LINEAR etc.
//        settings.maxFilter = GL_LINEAR;				// GL_NEAREST, GL_LINEAR etc.
//        settings.numSamples = 0;				// number of samples for multisampling (set 0 to disable)
//        maskFbo.allocate(settings);
//
//        im.update();
//        maskFbo.begin();
//        ofSetColor(255);
//        im.draw(0,0,im.getWidth(),im.getHeight());
//        maskFbo.end();
//

        maskImg = im;
        maskResolution->setValue(ofVec2f(maskImg.getWidth(),maskImg.getHeight()));
    }
    ActionParameter::Ptr setMaskPath;
    StringParameter::Ptr maskPath;
    FloatParameter::Ptr maskThreshold,invertMask;
    NumericParameter<ofVec2f>::Ptr maskResolution;
    ofFbo maskFbo;
    ofImage maskImg;

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

    addAndRegisterType<ShaderBase>(nodes, "mirror");
    addAndRegisterType<ShaderBase>(nodes, "champi");
    addAndRegisterType<CurveShader>(nodes);
    addAndRegisterType<KaleidoscopeShader>(nodes);
    addAndRegisterType<MaskShader>(nodes);
//



    soloShader(availableShaders["champi"]);

}


///////////////////////
//// Utils
////////////////////



const map<string,ShaderBase::DefaultUniform> ShaderBase::reservedUniformsMap {{"resolution",ShaderBase::DefaultUniform::resolution},{"time",ShaderBase::DefaultUniform::time},{"mouse",ShaderBase::DefaultUniform::mouse}};

void ShaderBase::autoParseUniforms() {
    if (!isLoaded()) return;
    clearParams();
    string source = shader.getShaderSource(GL_FRAGMENT_SHADER);
    defaultUniformFlags = 0;
    ofStringReplace(source, "\r", "");
    const auto lines = ofSplitString(source, "\n", true, true);
    for (const auto & l : lines) {
        const auto words = ofSplitString(l, " ", true, true);
        if (words.size() >= 3) {
            if (words[0] == "uniform" && words[1].find("sampler") == -1) {

                bool isFloat = words[1] == "float";
                bool isVec = words[1] == "vec2";
                const string pname = ofSplitString(words[2], ";", true, true)[0];
                const auto & reserved = reservedUniformsMap.find(pname);
                if(reserved!=reservedUniformsMap.end()){
                    setDefaultUniform(reserved->second);
                    continue;
                }
                string defaultV = "";
                if (isFloat || isVec ) {
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
                    else {
                        auto spl = ofSplitString(defaultV, ",");
                        while (spl.size() < 2) {spl.push_back("0");}
                        vp = addvP(pname, ofVec2f(ofToFloat(spl[0]), ofToFloat(spl[1])));
                    }
                    
                }
            }
        }
    }
}
