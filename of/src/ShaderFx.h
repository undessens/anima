#pragma once

#include "ofMain.h"
#include "AppConfig.h"



#include "ofParameterContainerWarper.h"



class ShaderBase : public ParameterContainer {
public:
    typedef Node::PtrT<ShaderBase> Ptr;
    typedef PCollection<FloatParameter> FloatParameterListType;
    typedef PCollection<IntParameter> IntParameterListType;
    typedef NumericParameter<ofVec2f> ofVec2fParameter ;
    typedef PCollection<ofVec2fParameter> Vec2ParameterListType;

    ShaderBase(const string & _name): ParameterContainer(_name),currentTime(0) {
        enabled = addParameter<BoolParameter>("enabled", false);
        order = addParameter<IntParameter>("order", 0);
    }
    virtual ~ShaderBase() {
    };

    ofShader shader;
    typedef enum {
        resolution = 0,
        mouse = 1,
        time = 2,
        speed = 3
    } DefaultUniform;
    int defaultUniformFlags;
    static const map<string, ShaderBase::DefaultUniform> reservedUniformsMap;
    bool hasDefaultUniform(DefaultUniform  s) {return (defaultUniformFlags >> (int)s) & 1;}
    bool setDefaultUniform(DefaultUniform  s) {
        if (s == speed) {speedParameter = addParameter<FloatParameter>("speed", 1.0);}
        return defaultUniformFlags |= (1 << (int)s);
    }
    FloatParameterListType fParams;
    Vec2ParameterListType vParams;
    FloatParameterListType customfParams;
    Vec2ParameterListType customvParams;
    IntParameterListType defineParams;

    BoolParameter::Ptr enabled;
    IntParameter::Ptr order;

    bool load(bool parseUniforms = true) {
        ofShaderSettings settings;

        auto renderer = ofGetGLRenderer();
#ifdef TARGET_RASPBERRY_PI
        string versionHeader = "";
#else
        string versionHeader = string("#version ") + ofGLSLVersionFromGL(renderer->getGLVersionMajor(), renderer->getGLVersionMinor()) + "\n";
#endif

        settings.shaderSources[GL_FRAGMENT_SHADER ] = versionHeader + ofBufferFromFile("shaders/" + getName() + ".frag").getText();
        settings.shaderSources[GL_VERTEX_SHADER ] = versionHeader + ofBufferFromFile("shaders/default.vert").getText();
        for (auto &d : defineParams.mapIterator()) {settings.intDefines[d.first] = d.second->getValue();}
        settings.intDefines["USE_ARB"] = USE_ARB;
#ifdef TARGET_RASPBERRY_PI
        settings.intDefines["TARGET_RASPBERRY_PI"] = 1;
#endif
        settings.sourceDirectoryPath = ofToDataPath("shaders/");
        settings.bindDefaults = true;


        shader.setup(settings);

        // shader.load("shaders/" + getName());
        string s = stateToString();
        if (parseUniforms) {
            autoParseUniforms();
        }
        initParams();
        setStateFromString(s);
        return shader.isLoaded();
    }

    bool reload(bool parseUniforms = true) {shader.unload() ; return load(parseUniforms);}

    void begin(const ofVec2f & resolution, const float deltaT) {
        bool needReload = false;
        for (auto d : defineParams.vIterator()) {
            if (d->hasChanged(true)) {needReload = true;}
        }
        if (needReload) {reload(false);}
        shader.begin(); setUniforms(resolution, deltaT);
    }
    void end() {shader.end();}
    bool isLoaded() {return shader.isLoaded();}

    void clearParams() {
        leaves.clearElementsInVec(fParams.vIterator());
        fParams.clear();
        leaves.clearElementsInVec(vParams.vIterator());
        vParams.clear();
        leaves.clearElementsInVec(customfParams.vIterator());
        customfParams.clear();
        leaves.clearElementsInVec(customvParams.vIterator());
        customvParams.clear();
        leaves.clearElementsInVec(defineParams.vIterator());
        defineParams.clear();


    }

    virtual void drawDbg() {};

protected:

    FloatParameterListType::ElemPtr addfP(const string & name, float v) {auto p = addParameter<FloatParameter>(name, v); fParams.addToCollection(p); return p;}
    Vec2ParameterListType::ElemPtr addvP(const string & name, ofVec2f v) {auto p = addParameter<ofVec2fParameter>(name, v); vParams.addToCollection(p); return p;}
    FloatParameterListType::ElemPtr addCustomfP(const string & name, float v) {auto p = addParameter<FloatParameter>(name, v); customfParams.addToCollection(p); return p;}
    Vec2ParameterListType::ElemPtr addCustomvP(const string & name, ofVec2f v) {auto p = addParameter<ofVec2fParameter>(name, v); customvParams.addToCollection(p); return p;}
    IntParameterListType::ElemPtr addDefineP(const string & name, int v) {auto p = addParameter<IntParameter>(name, v); defineParams.addToCollection(p); return p;}

    virtual void initParams() {};
    virtual void updateParams(float deltaT) {};
    virtual void update() {};

    virtual void setUniforms(const ofVec2f & resolution, const float deltaT) {
        if (hasDefaultUniform(DefaultUniform::resolution)) {shader.setUniform2f("resolution", resolution.x, resolution.y);}
        if (hasDefaultUniform(DefaultUniform::mouse)) {shader.setUniform2f("mouse", ofGetMouseX() * 1.0 / ofGetWidth(), ofGetMouseY() * 1.0 / ofGetHeight());}
        if (hasDefaultUniform(DefaultUniform::time)) {
            float dt = deltaT;
            if (hasDefaultUniform(DefaultUniform::speed)) {dt *= speedParameter->getValue();}
            currentTime += dt;
            shader.setUniform1f("time", currentTime);
        }
        updateParams(deltaT);
        for (const auto & p : fParams.vIterator()) {shader.setUniform1f(p->getName(), p->getValue());}
        for (const auto & p : vParams.vIterator()) {const ofVec2f & v ( p->getValue()); shader.setUniform2f(p->getName(), v.x, v.y);}
    };

private:
    void autoParseUniforms(); // internal utility function parsing frag shader for uniforms (float or vec2) one can define defaultValues : // (value) or (x,y) or (disabled)
    friend class ShaderFx;
    float currentTime;
    FloatParameter::Ptr speedParameter;

};







class ShaderFx : public ParameterContainer {
public:
    ShaderFx():
        ParameterContainer("shaders"),
        curShaderIdx(0),
        bShouldProcess(true),
        availableShaders(this->nodes),
        enabledShaders(*this) {

        curShaderName = addParameter<StringParameter>("shaderName", "");
        bDrawDbg = addParameter<BoolParameter>("drawDbg", false);

        addTrigger("next", [this](TriggerParameter *) {nextShader();});
        addTrigger("reload", [this](TriggerParameter *) {reload();});
        addTrigger("print", [this](TriggerParameter *) {DBG(toNiceString());});



    }
    void setup();
    void update()                     {for (auto & s : enabledShaders) {s->update();}}


    void draw(const ofTexture & tex, const float deltaT) {
        if (!shouldProcess()) {return;}
        ofRectangle targetR {0, 0, (float)ofGetWidth(), (float)ofGetHeight()};
        ofVec2f resolution {targetR.getWidth(), targetR.getHeight()};
        int numShaders = getNumShadersEnabled();
        if (numShaders == 0) {
            tex.draw(targetR );
        }
        else if (numShaders == 1) {
            const ShaderBase::Ptr  s = *(enabledShaders.begin());
            s->begin(resolution, deltaT);
            tex.draw(targetR.getX(), targetR.getY(), targetR.getWidth(), targetR.getHeight());
            s->end();

        }
        else {
            pingPong.allocateIfNeeded(targetR.getWidth(), targetR.getHeight());
            int i = 0;
            for (auto s : enabledShaders) {
                bool isLast = i == enabledShaders.size() - 1;
                if (!isLast)    {pingPong.getFront()->begin();}

                s->begin(resolution, deltaT);

                if (i == 0) {
                    tex.draw(targetR.getX(), targetR.getY(), targetR.getWidth(), targetR.getHeight());
                }
                else        {
                    pingPong.getBack()->draw(targetR.getX(), targetR.getY(), targetR.getWidth(), targetR.getHeight());
                }

                s->end();

                if (!isLast)    {pingPong.getFront()->end();}
                pingPong.swap();
                i++;
            }
        }
    }

    void nextShader()                 {
        curShaderIdx++; curShaderIdx %= availableShaders.size();
        soloShader(availableShaders[curShaderIdx]);
        ofLog() << currentShader->toNiceString();
    }

    string getInfo()     {return toNiceString();}//if(!currentShader.get()) return "no shader loaded";return "shader  : " + currentShader->getName() + (currentShader->isLoaded()?"":"not") + " loaded : \n" + currentShader->toNiceString();}


    bool soloShader(ShaderBase::Ptr s)   {
        currentShader = s;
        curShaderName->setValue(currentShader.get() ? currentShader->getName() : "none" , this);
        for (auto v : availableShaders.getNamedPtrSet().vIterator()) {
            v->enabled->setValue(v == currentShader);
        }
        return (currentShader->isLoaded() || currentShader->load());
    }

    bool reload() {
        bool res = true;
        for (auto v : availableShaders.getNamedPtrSet().vIterator()) {
            if (v.get() && v->enabled->getValue())res &= v->reload();
            else {res = false;}
        }
        return res;
    }


    bool shouldProcess()              {return bShouldProcess;}
    int getNumShadersEnabled() {return enabledShaders.size();}

    void parameterValueChanged(ParameterBase * ori) {
        if (ori == curShaderName.get()) {
            auto shaderToLoad = nodes.getWithTypeNamed<ShaderBase>(curShaderName->getValue());;
            if (shaderToLoad) {soloShader(shaderToLoad);}
            else {DBGE("no shader found for " << curShaderName->getValue())}
        }
    }


    void drawDbg() {if (currentShader && bDrawDbg->getValue()) {currentShader->drawDbg();}}

private:

    ShaderBase::Ptr currentShader;
    Parameter<string>::Ptr curShaderName;
    BoolParameter::Ptr bDrawDbg;

    struct PingPongBuffer {
        PingPongBuffer() : alloW(0), alloH(0) {}
        void allocateIfNeeded(int w, int h) {
            if (alloW != w || alloH != h) {
                front.allocate(w, h);
                back.allocate(w, h);
                alloW = w;
                alloH = h;
            }
        };
        void swap() {isSwapped = !isSwapped;};
        ofFbo * getFront() {return isSwapped ? &back : &front;}
        ofFbo * getBack() {return isSwapped ? &front : &back;}

        bool isSwapped;
        ofFbo front, back;
        int alloW, alloH;
    };
    PingPongBuffer pingPong;


    struct ShaderEnabledListOrdered :
        public std::vector<ShaderBase::Ptr>,
        public TypedView< ShaderBase, Node>::Listener,
        private ParameterBase::Listener {

        typedef std::vector<ShaderBase::Ptr> OrderedSetType;
        ShaderEnabledListOrdered(ShaderFx & _owner):
            TypedView< ShaderBase, Node>::Listener("ShaderEnabledList"),
            ParameterBase::Listener("shaderEnableListener"),
            owner(_owner) {
            _owner.availableShaders.listeners.add(this);
        };

    private:
        void valueChanged(ParameterBase* p, void* notifier)final {
            auto c = p->getParent(); if (c) {
                auto relEnSh = owner.availableShaders.getNamedPtrSet().findFirst<ShaderBase>([p](ShaderBase * s) {return (bool)(s && p == s->enabled.get());});
                if (relEnSh) {
                    auto it = std::find(begin(), end(), relEnSh);
                    if (it != OrderedSetType::end()) {OrderedSetType::erase(it);}
                    if (relEnSh->enabled->getValue()) {OrderedSetType::push_back(relEnSh);}

                }
                auto relOrdSh = owner.availableShaders.getNamedPtrSet().findFirst<ShaderBase>([p](ShaderBase * s) {return (bool)(s && p == s->order.get());});
                if (relOrdSh || relEnSh) {
                    std::sort(begin(), end(),
                    [](const ShaderBase::Ptr a, const ShaderBase::Ptr b) {
                        return *(a->order) < *(b->order);
                    });
                }


            }};
        void typedElementAdded(const Node::PtrT<ShaderBase> & s )final{s->enabled->listeners.add(this); s->order->listeners.add(this);};
        void typedElementRemoved(const Node::PtrT<ShaderBase> & s )final{s->enabled->listeners.remove(this); s->order->listeners.remove(this);};
        const ShaderFx & owner;
    };

    int curShaderIdx;
    bool bShouldProcess;


    TypedView<ShaderBase, Node> availableShaders;
    ShaderEnabledListOrdered enabledShaders;

};





