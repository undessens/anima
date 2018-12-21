#pragma once

// namespace std{
// static string to_string(const ofVec2f & v){
// return  string("["+std::to_string(v.x)+std::to_string(v.y)+"]");
//         }
// }
template <class T>
ostream & operator<<(ostream & o,const ofxOscMessage & m){
  o << m.getAddress();
  o << "types : " << m.getTypeString();
  // for (int i = 0 ; i < m.getNumArgs() ; i++){
  //   if()
  //   o << "," << e;
  // }
  return o;
}


#include "ParameterContainer.hpp"
class OSCParameterBinder{
public:
  OSCParameterBinder(ParameterContainer & _owner):owner(_owner){

  }
  typedef enum{
    None=-1,
    isX=-2,
    isY=-3,
    isZ=-4
  }SpecialAccessor;

  bool processOSC(const ofxOscMessage & m,int fromLevel = 0){
    auto spl = ofSplitString(m.getAddress(),"/",true,true);
    if(spl.size()<=fromLevel) return false;
    if(fromLevel>0)spl.erase(spl.begin(),spl.begin()+fromLevel);
    string leafName = spl[spl.size()-1];
    spl.resize(spl.size()-1);
    SpecialAccessor acc=leafName=="x"?isX:(leafName=="y"?isY:None);
    if(acc!=None){
      leafName = spl[spl.size()-1];
      spl.resize(spl.size()-1);
    }
    // DBG("processing osc : " << spl);

    auto * insp = &owner;
    for(auto & cn:spl){
      if(!insp){break;}
      insp = insp->nodes.getWithTypeNamed<ParameterContainer>(cn).get();
    }
    if(insp){
      auto p = insp->leaves.getWithTypeNamed<ParameterBase>(leafName).get();
      if(p){
        
        bool hasSet =  setParameterFromOSC(m,p,acc);
        if(!hasSet){
          DBG("wrong OSC message formating for " << p->getName() << m);
        }
      }
    }
    else{
      DBG("container not found for " << m.getAddress() << "::" << spl);
    }
    return false;
  }

private:
  bool setParameterFromOSC(const ofxOscMessage & msg,ParameterBase * p,const SpecialAccessor & acc){
  if(auto np =dynamic_cast<TriggerParameter*>(p)){np->trigger();return true;}
  if(msg.getNumArgs()==0){
    if(auto np =dynamic_cast<Parameter<bool>*>(p)){np->setValue(!np->getValue());return true;}
  }
  else if(msg.getNumArgs()==1){// check /myname value
    if(acc==None){
      if(auto np =dynamic_cast<Parameter<float>*>(p)){np->setValue(msg.getArgAsFloat(0));return true;}
      else if(auto np =dynamic_cast<Parameter<int>*>(p)){np->setValue(msg.getArgAsInt(0));return true;}
      else if(auto np =dynamic_cast<Parameter<double>*>(p)){np->setValue(msg.getArgAsFloat(0));return true;}
      else if(auto np =dynamic_cast<Parameter<bool>*>(p)){np->setValue(msg.getArgAsFloat(0));return true;}
    }
    else if(acc==isX || acc==isY || acc==isZ){
      if(auto np =dynamic_cast<Parameter<ofVec2f>*>(p)){
        if(acc==isZ){return false;}
        auto copy = np->getValue();
        copy[acc==isX?0:1] = msg.getArgAsFloat(0);
        np->setValue(copy);
        return true;
        
      }
      else if(auto np =dynamic_cast<Parameter<ofVec3f>*>(p)){
        auto copy = np->getValue();
        copy[acc==isX?0:acc==isY?1:2] = msg.getArgAsFloat(0);
        np->setValue(copy);
        return true;
      }
    }

  }
  else if ( msg.getNumArgs()==2){
    if(auto np =dynamic_cast<Parameter<ofVec2f>*>(p)){
      DBG(np->getName() << msg);
      np->setValue(ofVec2f(msg.getArgAsFloat(0),msg.getArgAsFloat(1)));
      return true;}
  }
  else if ( msg.getNumArgs()==3){
    if(auto np =dynamic_cast<Parameter<ofVec3f>*>(p)){np->setValue(ofVec3f(msg.getArgAsFloat(0),msg.getArgAsFloat(1),msg.getArgAsFloat(2)));return true;}
  }
  

  return false;
}
private:
const ParameterContainer & owner;
};

