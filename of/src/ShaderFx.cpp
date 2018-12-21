#include "AppConfig.h"

#include "ShaderFx.h"




class KaleidoscopeShader : public ShaderBase{
        public:
        
        KaleidoscopeShader():ShaderBase("kaleidoscope"),
        minAngularVelocity(.10){
        }

        void initParams()final{
                vAngle = addCustomfP("vAngle",0);
                debug = addCustomfP("debug",0);
                _angle  = fParams.getRef("rotation");
                // tstV = leaves.createWithType<Parameter< vector<float> > >("testV",vector<float>({2.f,-2.3f}) );
                
        };


        void updateParams(float deltaT) final{
                // float distToEnd = abs(fmod(angle+PI,2*PI)-PI);
                // auto vAngle = vAngle.lock();
                auto & vA = vAngle->getValue();//.get();
                auto & angle = *_angle.get();
                if(vA != 0){
                        bool shouldStop = abs(vA)<=minAngularVelocity ;
                // float tst =  abs((float)angle);
                if(!shouldStop || angle!=0){
                        angle+=(float)((vA>0?1:-1)*MAX(abs(vA),minAngularVelocity) *2*PI*deltaT);        
                        bool hasLooped = false;
                        while(angle>2*PI){angle-=2*PI;hasLooped = true;}
                        while(angle<0){angle+=2*PI;hasLooped=true;}
                        if(shouldStop&& hasLooped){angle=0;}
                }
                else{
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

class CurveShader : public ShaderBase{
    public:
    const int textureRes = 256;
    CurveShader():ShaderBase("ShadowHighlights"){
        cTex.allocate(textureRes,1,GL_RGBA,false);
        cTex.readToPixels(pixBuf);
    }
    void initParams()final{
                low = addCustomvP("lowT",ofVec2f(0.2));
                high = addCustomvP("highT",ofVec2f(0.8));
                populateTexture();
    };
    // bezier

    float interpolatePart(float x, ofVec2f minP,ofVec2f maxP,float minT,float maxT){
    float delta = maxP.x-minP.x;
    float t = (x-minP.x)/delta;
    float h00 = (1+2*t)*(1-t)*(1-t);
    float h10 = t*(1-t)*(1-t);
    float h01 = t*t*(3-2*t);
    float h11 = t*t*(t-1);
    
    return h00*minP.y + delta*h10*minT + h01*maxP.y+h11*delta*maxT;
    }
    void populateTexture(){

        auto p1 = *low.get()*textureRes;
        auto p2 = *high.get()*textureRes;
        // DBG("populating texture : " << low->getValue() << ":::" << high->getValue());
        
        float initT = p1.y/p1.x;
        float lastT = (1-p2.y)/(1-p2.x);
        float lowT = initT/2.0 + .5*(p2.y-p1.y)/(p2.x-p1.x);
        float highT = lowT/2.0 + lastT/2.0;
        // ostringstream os;
        
        for(int i = 0 ; i <textureRes  ; i++){
            float v ;
            if(i<p1.x){
                 v =interpolatePart(i,ofVec2f(0),p1,initT,lowT);
            }
            else if(i<p2.x){
                v = interpolatePart(i,p1,p2,lowT,highT);
            }
            else{
                v = interpolatePart(i,p2,ofVec2f(1),highT,lastT);   
            }
            v*=255.0/textureRes;
            float clamped = ofClamp(v,0,255);
            auto col = ofColor(clamped);
            if(v<0){col[1]=255;}
            if(v>255){col[2]=0;}
            // os <<(int)v << ", ";
            pixBuf.setColor(i,0,col);
            // pixBuf.setColor(i,1,col);
        }
        // DBG(os.str());
        cTex.loadData(pixBuf);

    }

    void updateParams(float deltaT)final{
        if(low->hasChanged(true) || high->hasChanged(true)){
            populateTexture();
        }
        shader.setUniformTexture("curveTex",cTex,1);
    }

    void drawDbg()final{
        cTex.draw(0,0,textureRes,10);
    }

    ofTexture cTex;
    Vec2ParameterListType::ElemPtr low;
    Vec2ParameterListType::ElemPtr high;
    ofPixels pixBuf;
};


template<class T,class CONTAINER,class ...Args>
void addAndRegisterType (CONTAINER & cont,Args... args) { 
        auto inst = new T(args...);
        cont.add(inst);
        ParameterContainerFactory::i().registerType<T>("shader_"+inst->getName(),args...); 
        // populate ShaderFx with shaderParameters
        inst->load();
        // inst->unload();
}

void ShaderFx::setup(){

        addAndRegisterType<ShaderBase>(nodes,"mirror");
        addAndRegisterType<CurveShader>(nodes);
        addAndRegisterType<KaleidoscopeShader>(nodes);
        // nodes.add(new ShaderBase("mirror"));
        // nodes.add(new KaleidoscopeShader());

        
    loadShader(getAvailableShaders()[0]);

}


///////////////////////
//// Utils
////////////////////


bool isReservedUniformName(const string & n){return n == "resolution" || n== "mouse" || n=="time";}

void ShaderBase::autoParseUniforms(){
        if(!isLoaded()) return;
        clearParams();
        string source = shader.getShaderSource(GL_FRAGMENT_SHADER);
        ofStringReplace(source,"\r","");
        const auto lines =ofSplitString(source,"\n",true,true);
        for(const auto & l:lines){
                const auto words = ofSplitString(l," ",true,true); 
                if(words.size()>=3){
                        if(words[0]=="uniform" && words[1].find("sampler")==-1){
                                bool isFloat = words[1]=="float";
                                bool isVec = words[1]=="vec2";
                                const string pname = ofSplitString(words[2],";",true,true)[0];
                                string defaultV = "";
                                if(isFloat || isVec ){
                                        int commStart = l.find("//");
                                        Node::PtrT<ParameterBase> vp(nullptr);
                                        if(commStart!=-1){
                                                defaultV = l.substr(commStart);
                                                int endP = defaultV.find(")");
                                                int startP = defaultV.find("(");
                                                if(startP!=1 && endP!=-1 && endP>startP ){
                                                        defaultV = defaultV.substr(startP+1,endP-startP-1);
                                                }
                                                else{
                                                        defaultV = "";
                                                }
                                                if(defaultV=="disabled"){
                                                        continue;
                                                }
                                        }
                                        if(isFloat){
                                                vp = addfP(pname,ofToFloat(defaultV));
                                        }
                                        else{
                                                auto spl = ofSplitString(defaultV,",");
                                                while(spl.size()<2){spl.push_back("0");}
                                                vp = addvP(pname,ofVec2f(ofToFloat(spl[0]),ofToFloat(spl[1])));
                                        }
                                        if(vp && isReservedUniformName(vp->getName())){
                                                vp->isSavable = false;
                                        }

                                }
                        }
                }
        }
}
