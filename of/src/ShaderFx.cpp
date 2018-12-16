#include "ShaderFx.h"



void ShaderBase::autoParseUniforms(){
        if(!isLoaded()) return;
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
                                if(isFloat || isVec){
                                        int commStart = l.find("//");
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
                                        }
                                        if(isFloat){
                                                addfP(pname,ofToFloat(defaultV));
                                        }
                                        else{
                                                auto spl = ofSplitString(defaultV,",");
                                                while(spl.size()<2){spl.push_back("0");}
                                                addvP(pname,ofVec2f(ofToFloat(spl[0]),ofToFloat(spl[1])));
                                        }
                                }
                        }
                }
        }
}


class KaleidoscopeShader : public ShaderBase{
        public:
        
        KaleidoscopeShader():ShaderBase("kaleidoscope"),
        minAngularVelocity(.10){
        }

        void initParams()final{
                clearParams();
                autoParseUniforms();
                vAngle = addCustomfP("vAngle",0);
                debug = addCustomfP("debug",0);
                angle  = fParams.getRef("rotation");
                ofLog() << pToString();
        };


        void updateParams(float deltaT) final{
                // float distToEnd = abs(fmod(angle+PI,2*PI)-PI);
                
                bool shouldStop = abs(vAngle->get())<=minAngularVelocity && vAngle->get()!=0;
                
                if(!shouldStop || *angle!=0){
                        *angle+=(vAngle>0?1:-1)*MAX(abs(vAngle->get()),minAngularVelocity) *2*PI*deltaT;        
                        bool hasLooped = false;
                        while(*angle>2*PI){*angle-=2*PI;hasLooped = true;}
                        while(*angle<0){*angle+=2*PI;hasLooped=true;}
                        if(shouldStop&& hasLooped){debug->set(*angle);*angle=0;}
                }
                else{
                        *angle = 0;
                }
        }
        bool processOSCMessage(const ofxOscMessage & msg,const vector<string> & splitted,int idx)final{
            if(ShaderBase::processOSCMessage(msg,splitted,idx)) return true;


            return false;

    };

    CustomP<float> * vAngle;
    CustomP<float> * debug;

    float minAngularVelocity;
    
    float * angle;
    


};



void ShaderFx::setup(){

    getAvailableShaders().push_back(new KaleidoscopeShader());
    loadShader(getAvailableShaders()[0]);

}





string toIndentedString(const string & ins,int numSpaces){
        if(numSpaces==0){return ins;}
        int i = 0 ;
        char c;
        string res;
        int odepth = -1;
        int adepth = -1;
        string spaceTab ="";
        while(i < ins.size()){
                
                c = ins[i];
                
                if(c =='{'){
                        res+=c;
                        odepth++;
                        spaceTab="";for(int i = 0 ; i < numSpaces*(odepth+1) ; i++){spaceTab += " ";}
                        res+="\n" + spaceTab;
                        
                }

                else if(c == '}'){
                        res+="\n";
                        odepth--;
                        spaceTab="";for(int i = 0 ; i < numSpaces*(odepth+1) ; i++){spaceTab += " ";}
                        res+=spaceTab+c;
                        
                }
                else {
                        res+=c;
                        if(c == '['){
                        adepth++;
                        }
                        else if(c == ']'){
                                adepth--;
                        }
                        else if(c==',' && adepth<0){
                         res+="\n" + spaceTab;
                        }
                }

                i++;
        }
        return res;

}
