#pragma once

#include "ofMain.h"
#include "ofxOsc.h"



#include "ofParameterContainerWarper.h"

class ShaderBase : public ParameterContainer {
public:
  typedef Node::PtrT<ShaderBase> Ptr;
  typedef PCollection<FloatParameter> FloatParameterListType;
  typedef NumericParameter<ofVec2f> ofVec2fParameter ;
  typedef PCollection<ofVec2fParameter> Vec2ParameterListType;
  ShaderBase(const string & _name): ParameterContainer(_name) {

  }
  virtual ~ShaderBase() {};

  ofShader shader;
  FloatParameterListType fParams;
  Vec2ParameterListType vParams;
  FloatParameterListType customfParams;
  Vec2ParameterListType customvParams;

  bool load() {
    shader.load("shaders/" + getName());
    autoParseUniforms();
    initParams();

    return shader.isLoaded();
  }

  bool reload() {shader.unload() ; return load();}

  void begin(const ofVec2f & resolution, const float deltaT) {shader.begin(); setUniforms(resolution, deltaT);}
  void end() {shader.end();}
  bool isLoaded() {return shader.isLoaded();}

  void clearParams() {
    fParams.clear(); vParams.clear(); customfParams.clear(); customvParams.clear();
    clearNode();
  }

  virtual void drawDbg() {};

protected:

  FloatParameterListType::ElemPtr addfP(const string & name, float v) {auto p = addParameter<FloatParameter>(name, v); fParams.addToCollection(p); return p;}
  Vec2ParameterListType::ElemPtr addvP(const string & name, ofVec2f v) {auto p = addParameter<ofVec2fParameter>(name, v); vParams.addToCollection(p); return p;}
  FloatParameterListType::ElemPtr addCustomfP(const string & name, float v) {auto p = addParameter<FloatParameter>(name, v); customfParams.addToCollection(p); return p;}
  Vec2ParameterListType::ElemPtr addCustomvP(const string & name, ofVec2f v) {auto p = addParameter<ofVec2fParameter>(name, v); customvParams.addToCollection(p); return p;}


  virtual void initParams() {};
  virtual void updateParams(float deltaT) {};
  virtual void update() {};

  virtual void setUniforms(const ofVec2f & resolution, const float deltaT) {
    vParams.setIfContains("resolution" , resolution);
    vParams.setIfContains("mouse" , ofVec2f(ofGetMouseX() * 1.0 / ofGetWidth(), ofGetMouseY() * 1.0 / ofGetHeight()));
    fParams.setIfContains("time", ofGetElapsedTimef());
    updateParams(deltaT);
    for (const auto & p : fParams.vectorIterator()) {shader.setUniform1f(p->getName(), p->getValue());}
    for (const auto & p : vParams.vectorIterator()) {const ofVec2f & v ( p->getValue()); shader.setUniform2f(p->getName(), v.x, v.y);}
  };

private:
  void autoParseUniforms(); // internal utility function parsing frag shader for uniforms (float or vec2) one can define defaultValues : // (value) or (x,y) or (disabled)
  friend class ShaderFx;

};







class ShaderFx : public ParameterContainer {
public:
  ShaderFx(): ParameterContainer("Shaders"), curShaderIdx(0), bShouldProcess(true), oscBind(*this) {

    curShaderName = leaves.createWithType<Parameter<string>>("shaderName", "");
    bDrawDbg = leaves.createWithType<Parameter<bool>>("drawDbg", false);

    addTrigger("next", [this](TriggerParameter *) {nextShader();});
    addTrigger("reload", [this](TriggerParameter *) {reload();});
    addTrigger("print", [this](TriggerParameter *) {DBG(toNiceString());});


  }
  void setup();
  void update()                     {if (shouldProcess()) {currentShader->update();}}

  void begin(const ofVec2f & resolution, const float deltaT)    {if (shouldProcess()) {currentShader->begin(resolution, deltaT);}}
  void end()                        {if (shouldProcess()) {currentShader->end();}}

  void nextShader()                 {curShaderIdx++; curShaderIdx %= getAvailableShaders().size(); loadShader(getAvailableShaders()[curShaderIdx]); ofLog() << currentShader->toNiceString(); }

  string getCurrentShaderInfo()     {return toNiceString();}//if(!currentShader.get()) return "no shader loaded";return "shader  : " + currentShader->getName() + (currentShader->isLoaded()?"":"not") + " loaded : \n" + currentShader->toNiceString();}


  bool loadShader(ShaderBase::Ptr s)   {currentShader = s; curShaderName->setValue(currentShader.get() ? currentShader->getName() : "none"); return currentShader.get() && (currentShader->isLoaded() || currentShader->load());}

  bool reload()                     {if (currentShader.get()) {return  currentShader->reload();} return false;}

  bool isLoaded()                   {return currentShader && currentShader->isLoaded();}
  bool shouldProcess()              {return isLoaded() && bShouldProcess;}

  bool processOSCMessage(const ofxOscMessage & m, const vector<string> & splitted, int idx) {return oscBind.processOSC(m, idx);}



  void savePreset(string path) {
    ofFile f(path, ofFile::Mode::WriteOnly);
    if (!f.exists()) {f.create();}
    auto nodeView = createNodeView<ParameterBase>(
    [](ParameterBase * c) {return c->isSavable;},
    [this](Node * n) {
      if (auto s = dynamic_cast<ShaderBase*>(n)) {return s->getName() == curShaderName->getValue();}
      return true;
    });

    ofBuffer buf; buf.set(nodeView->toNiceString());
    f.writeFromBuffer(buf);
  }
  void loadPreset(string path) {
    ofFile f(path, ofFile::Mode::ReadOnly);
    if (!f.exists()) {ofLogError() << "no file at " << path; return ;}
    ofBuffer buf(f.readToBuffer());
    stateFromString(buf.getText());
  }
  void drawDbg() {if (currentShader && bDrawDbg->getValue()) {currentShader->drawDbg();}}

private:

  ShaderBase::Ptr currentShader;
  Parameter<string>::Ptr curShaderName;
  Parameter<bool>::Ptr bDrawDbg;


  int curShaderIdx;
  bool bShouldProcess;
  OSCParameterBinder oscBind;
  const std::vector<ShaderBase::Ptr> getAvailableShaders() {return nodes.getWithTypeSafe<ShaderBase>();}


};





