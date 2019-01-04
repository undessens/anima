#pragma once


#include "ParameterContainer.hpp"
#include "ofxOsc.h"


// force ofVectors to be enclosed in brackets
template<class T>
static T ofVecFromString(const string & s) {
  if (s == "")return {};
  int start = s.find('[');
  int end = s.find(']');
  if (start == string::npos || end == string::npos || end == start) {return{};}
  auto sub = s.substr(start + 1, end - start - 1);
  // DBG(sub);
  istringstream is(sub);
  is >> std::noskipws;
  T res;
  is >> res;
  // DBG("res : " << res);
  return res;
}

namespace StringUtils {


template<>
struct ElemSerializer<ofVec2f> {
  static string toString(const ofVec2f & value) {return "[" + ofToString(value) + "]"; }
  static ofVec2f fromString(const string & s) {return ofVecFromString<ofVec2f>(s);}
};

template<>
struct ElemSerializer<ofVec3f> {
  static string toString(const ofVec3f & value) {return "[" + ofToString(value) + "]"; }
  static ofVec3f fromString(const string & s) {return ofVecFromString<ofVec3f>(s);}
};
}




class OSCParameterBinder : public ParameterContainer::Listener {
public:
  OSCParameterBinder() : ParameterContainer::Listener("OSCBinder"), processingParameter(nullptr) {}
  typedef enum {
    NoAccessor = -1,
    isX = -2,
    isY = -3,
    isZ = -4
  } SpecialAccessor;
  ofxOscSender oscSender;

  void setup(ParameterContainer::Ptr _owner, const string ip, int feedbackPort) {
    if (owner) {
      owner->parameterContainerListeners.remove(this);
    }
    owner = _owner;
    owner->parameterContainerListeners.add(this);
    oscSender.setup(ip, feedbackPort);
  }

  bool processOSC(const ofxOscMessage & m, int fromLevel ) {
    auto spl = ofSplitString(m.getAddress(), "/", true, true);
    if (spl.size() <= fromLevel) return false;
    if (fromLevel > 0)spl.erase(spl.begin(), spl.begin() + fromLevel);

    string leafName = spl[spl.size() - 1];
    if(spl.size()==1 && leafName=="getAll"){
      DBG("send all params");
      getAll();
      return true;
    }
    spl.resize(spl.size() - 1);
    SpecialAccessor acc = leafName == "x" ? isX : (leafName == "y" ? isY : NoAccessor);
    if (acc != NoAccessor) {
      leafName = spl[spl.size() - 1];
      spl.resize(spl.size() - 1);
    }
    // DBG("processing osc : " << spl);

    auto insp = owner;

    for (auto & cn : spl) {
      if (!insp) {break;}
      insp = insp->nodes.getWithTypeNamed<ParameterContainer>(cn);
    }
    if (insp) {
      auto p = insp->leaves.getWithTypeNamed<ParameterBase>(leafName);
      if (p) {
        processingParameter = p.get();
        bool hasSet =  setParameterFromOSC(m, p.get(), acc);
        processingParameter = nullptr;
        if (!hasSet) {
          DBGE("wrong OSC message formating for " << p->getName() << m);
        }
      }
      else {
          DBGE("parameter not found for " << m.getAddress());// << "::" << insp->toNiceString() );
      }
    }
    else {
      DBGW("container not found for " << m.getAddress() << "::" << spl);
    }
    return false;
  }

  void childParameterChanged(ParameterContainer* parent, ParameterBase * p) final{
    if (processingParameter == p) {return;}


    // sendOscMessageForParameter(p);

  }
  void sendOscMessageForParameter(ParameterBase * p) {

    ofxOscMessage m;
    if (fillMessageWithParameter(m, p)) {
      // DBG("sending msg" << m);
      oscSender.sendMessage(m, false);
    }
    else {
      DBGE("parameter not sent");
    }
  }

private:

  bool fillMessageWithParameter(ofxOscMessage & m, ParameterBase * p) {
    vector<string> path = p->getCurrentPath();
    string oscAddr = "/" + StringUtils::joinString(path, '/');
    m.setAddress(oscAddr);
    if (auto np = dynamic_cast<Parameter<float>*>(p)) {m.addFloatArg(np->getValue());}
    else if (auto np = dynamic_cast<Parameter<int>*>(p)) {m.addIntArg(np->getValue());}
    else if (auto np = dynamic_cast<Parameter<ofVec2f>*>(p)) {m.addFloatArg(np->getValue().x); m.addFloatArg(np->getValue().y);}
    else if (auto np = dynamic_cast<Parameter<ofVec3f>*>(p)) {m.addFloatArg(np->getValue().x); m.addFloatArg(np->getValue().y); m.addFloatArg(np->getValue().z);}
    else if (auto np = dynamic_cast<Parameter<bool>*>(p)) {m.addIntArg(np->getValue() ? 1 : 0);}
    else {return false;}
    return true;

  }
  void getAll() {
    owner->applyRecursively<ParameterBase>([this](ParameterBase * p) {sendOscMessageForParameter(p);});

  }
  bool setParameterFromOSC(const ofxOscMessage & msg, ParameterBase * p, const SpecialAccessor & acc) {
    auto commitSession = p->startScopedCommitingSession(this);
    if (auto np = dynamic_cast<TriggerParameter*>(p)) {np->trigger(); return true;}
    else if (auto np = dynamic_cast<ActionParameter*>(p)) {
      string command; for (int i = 0 ; i < msg.getNumArgs(); i++) {if (i != 0) {command += " ";} command += msg.getArgAsString(i);}
      np->executeAction(command); return true;
    }
    if (msg.getNumArgs() == 0) {
      if (auto np = dynamic_cast<Parameter<bool>*>(p)) {np->setValue(!np->getValue()); return true;}
    }
    else if (msg.getNumArgs() == 1) { // check /myname value
      if (acc == NoAccessor) {

        if (auto np = dynamic_cast<Parameter<float>*>(p)) {np->setValue(msg.getArgAsFloat(0)); return true;}
        else if (auto np = dynamic_cast<Parameter<int>*>(p)) {np->setValue(msg.getArgAsInt(0)); return true;}
        else if (auto np = dynamic_cast<Parameter<double>*>(p)) {np->setValue(msg.getArgAsFloat(0)); return true;}
        else if (auto np = dynamic_cast<Parameter<bool>*>(p)) {np->setValue(msg.getArgAsFloat(0)); return true;}
        else if (auto np = dynamic_cast<Parameter<string>*>(p)) {np->setValue(msg.getArgAsString(0)); return true;}


      }
      else if (acc == isX || acc == isY || acc == isZ) {
        if (auto np = dynamic_cast<Parameter<ofVec2f>*>(p)) {
          if (acc == isZ) {return false;}
          auto copy = np->getValue();
          copy[acc == isX ? 0 : 1] = msg.getArgAsFloat(0);
          np->setValue(copy);
          return true;

        }
        else if (auto np = dynamic_cast<Parameter<ofVec3f>*>(p)) {
          auto copy = np->getValue();
          copy[acc == isX ? 0 : acc == isY ? 1 : 2] = msg.getArgAsFloat(0);
          np->setValue(copy);
          return true;
        }
      }

    }
    else if ( msg.getNumArgs() == 2) {
      if (auto np = dynamic_cast<Parameter<ofVec2f>*>(p)) {
        np->setValue(ofVec2f(msg.getArgAsFloat(0), msg.getArgAsFloat(1)));
        return true;
      }
    }
    else if ( msg.getNumArgs() == 3) {
      if (auto np = dynamic_cast<Parameter<ofVec3f>*>(p)) {np->setValue(ofVec3f(msg.getArgAsFloat(0), msg.getArgAsFloat(1), msg.getArgAsFloat(2))); return true;}
    }


    return false;
  }
private:
  ParameterBase*  processingParameter;
  ParameterContainer::Ptr owner;
};

