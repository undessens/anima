#pragma once

#include "ofMain.h"
#include "ofxOsc.h"
#include <map>

extern string toIndentedString(const string & ins,int numSpaces);


template<class T>
class CustomP{
public:
       CustomP(const T & t):value(t){}; 
        const T & set(const T & t){bhasChanged |= t!=value;value = t; return value;}
        const T & get()const{return value;}
        bool hasChanged(bool clearFlag = true){if(bhasChanged){if(clearFlag){bhasChanged = false;}return true;} return false;}
        friend std::ostream& operator<<(std::ostream& os, const CustomP<T>& cp){os << cp.value;return os;}
        friend std::istream& operator>>(std::istream& is, const CustomP<T>& cp){T v;is >> v;cp.set(v);return is;}
        const T & operator =(const T & t){return set(t);}
        const T & operator ++(){set(value++);}
        const T & operator +(const T & o){return set(value+o);}
        const T & operator /(const T & o){return set(value/o);}
        operator const T&(){return value;}
private:
        T value;
        bool bhasChanged;


};


template<class T>
class PList : public map<string,T>{
public:
        void setIfContains(const string & name,const T & v){
                auto l = map<string,T>::find(name);
                if(l!=map<string,T>::end()){l->second = v;}
        }

        T* getRef(const string & name){return &map<string,T>::find(name)->second;}
        T * add(const string & n,const T & v){return &map<string,T >::emplace(n,v).first->second;}

        string toString(int indentSpace=0)const {return toIndentedString(ofToString(*this),indentSpace);}

        friend std::ostream& operator<<(std::ostream& os, const PList<T>& cp){
                const bool shouldEncloseInBrackets = std::is_same<T,ofVec2f>::value  || std::is_same<T,ofVec3f>::value  ;
                os <<"{";
                int i = cp.size()-1;
                for(const auto &v:cp){
                        os << v.first <<  ":" ;
                        if(shouldEncloseInBrackets)os<<"[";
                        os << v.second;
                        if(shouldEncloseInBrackets)os<<"]";
                        if(i!=0){os << ",";}
                        i--;
                }
                os << "}";
                return os;
        }
        
        friend std::istream& operator>>(std::istream& is, const PList<T>& cp){
                const bool shouldEncloseInBrackets = std::is_same<T,ofVec2f>::value  || std::is_same<T,ofVec3f>::value  ;
                const string ignore=" \t\r\n"; 
                int depth = -1;
                char c;
                string cs ,name;
                while(is >>c){
                        if(ignore.find(c))continue;
                        if(c=='{' ){depth++;}
                        if(c==':' && depth == 0){name = cs;}
                        if (c==',' && depth==0){
                                if(shouldEncloseInBrackets && cs.size()>0){cs=cs.substr(1,cs.size()-2);}
                                cp[name] = ofTo<T>(cs);
                                cs = "";name = "";
                        }
                        if(depth>=0){cs+=c;}
                        if(c=='}' && depth==0){break;}
                }
                if(name!="" && cs!=""){
                        if(shouldEncloseInBrackets && cs.size()>0){cs=cs.substr(1,cs.size()-2);}
                        cp[name] = ofTo<T>(cs);
                }

                return is;
        }

};







class ShaderBase{
public:
  ShaderBase(const string & _name):name(_name){
  }
  
  string name;
  ofShader shader;
  PList<float> fParams;
  PList<ofVec2f> vParams;
  PList<CustomP<float>> customfParams;

  bool load(){
        shader.load("shaders/"+name);
        initParams();
        return shader.isLoaded();
  }
  string getPreset(){

  }
  bool reload(){shader.unload() ; return load();}
  
  void begin(const ofVec2f & resolution,const float deltaT){shader.begin();setUniforms(resolution,deltaT);}
  void end(){shader.end();}
  bool isLoaded(){return shader.isLoaded();}

  void clearParams(){fParams.clear();vParams.clear();customfParams.clear();}
  string pToString(){
        const int ind = 4;
        return vParams.toString(ind) + "\n" + fParams.toString(ind) + "\n" + customfParams.toString(ind) + "\n";
}



protected:

  void addfP(const string & name,float v){fParams[name] = v;}
  void addvP(const string & name,ofVec2f v){vParams[name] = v;}
  CustomP<float> * addCustomfP(const string & name,float v){return customfParams.add(name,CustomP<float>(v));}

  void autoParseUniforms(); // utility function parsing frag shader for uniforms (float or vec2) one can define defaultValues : // (value) or (x,y)
  virtual void initParams()=0;
  virtual void updateParams(float deltaT){};
  virtual void update(){};
  virtual bool processOSCMessage(const ofxOscMessage & msg,const vector<string> & splitted,int idx){

    if(msg.getNumArgs()>=1){// check /myname value
      auto elem =  fParams.find(splitted[idx]);
      if(elem!=fParams.end()){elem->second = msg.getArgAsFloat(0);return true;}
      auto elem2= customfParams.find(splitted[idx]);
      if(elem2!=customfParams.end()){elem2->second.set( msg.getArgAsFloat(0));return true;}
    }
    if(msg.getNumArgs()>=2){ // check /myname x y

      auto velem =  vParams.find(splitted[idx]);
      if(velem!=vParams.end()){
        velem->second.set(msg.getArgAsFloat(0) , msg.getArgAsFloat(1));
        return true;
      }
    }
    if(msg.getNumArgs()==1 ){ // check /myname/x value
      bool isX = splitted[splitted.size()-1] == "x" ;
      bool isY = splitted[splitted.size()-1] == "y" ;
      if(isX || isY){
        auto velem =  vParams.find(splitted[idx]);
        if(velem!=vParams.end()){
          velem->second[isX?0:1] = msg.getArgAsFloat(0);
          return true;
        }

      }
    }
    return false;
    
  };

 static std::vector<ShaderBase*> & getAvailableShaders(){
    static std::vector<ShaderBase*> availableShaders;
    return availableShaders;
  }

  virtual void setUniforms(const ofVec2f & resolution,const float deltaT) {
    vParams.setIfContains("resolution" , resolution);
    vParams.setIfContains("mouse" , ofVec2f(ofGetMouseX()*1.0/ofGetWidth(),ofGetMouseY()*1.0/ofGetHeight()));
    fParams.setIfContains("time",ofGetElapsedTimef());
    for (const auto & p:fParams){shader.setUniform1f(p.first,p.second);}
    for (const auto & p:vParams){shader.setUniform2f(p.first,p.second.x,p.second.y);}
  };

  
  friend class ShaderFx;

};







class ShaderFx{
public:
  ShaderFx():curShaderIdx(0),bShouldProcess(true){}
  void setup();
  void update()                     {if(shouldProcess()){currentShader->update();}}

  void begin(const ofVec2f & resolution,const float deltaT)    {if(shouldProcess()){currentShader->begin(resolution,deltaT);}}
  void end()                        {if(shouldProcess()){currentShader->end();}}

  void nextShader()                 {curShaderIdx++;curShaderIdx%=getAvailableShaders().size();loadShader(getAvailableShaders()[curShaderIdx]);}

  string getCurrentShaderInfo()     {if(!currentShader) return "no shader loaded";return "shader  : " + currentShader->name + (currentShader->isLoaded()?"":"not") + " loaded : \n" + currentShader->pToString();}


  bool loadShader(ShaderBase * s)   {currentShader = s;return currentShader && (currentShader->isLoaded() || currentShader->load());}

  bool reload()                     {if(currentShader){return  currentShader->reload();}return false;}

  bool isLoaded()                   {return currentShader && currentShader->isLoaded();}
  bool shouldProcess()              {return isLoaded() && bShouldProcess;}
  bool processOSCMessage(const ofxOscMessage & m,const vector<string> & splitted,int idx){
    if(splitted.size()<idx+1) return false;
    if(splitted[idx] == "next"){nextShader();return true;}
    if(splitted[idx] == "reload"){reload();return true;}
    if(currentShader){return currentShader->processOSCMessage(m,splitted,idx );}
    return false;  
  }

private:

  ShaderBase * currentShader;
  int curShaderIdx;
  bool bShouldProcess;

  std::vector<ShaderBase*> & getAvailableShaders(){
    return ShaderBase::getAvailableShaders();
  }
  

};





