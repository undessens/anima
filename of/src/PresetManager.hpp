#include <iostream>
#include <fstream>
using namespace std;

// struct ExplicitlyPresetable {
//     virtual ~ExplicitlyPresetable() {}
//     virtual NodeView::Ptr getPresetView = 0;

// };
class PresetManager: public ParameterContainer {
public:
    PresetManager(ParameterContainer::Ptr rootParam ) : ParameterContainer("presets"),root(rootParam) {
        baseDirectory = "/tmp";
        addParameter<ActionParameter>("save", [this](const string & s) {
            // if (!root) {DBGE("preset root not set"); return;}
            auto it = s.find(' ');
            auto path = StringUtils::trim(s.substr(0, it));
            auto name = StringUtils::trim(s.substr(it + 1));
            auto pathSpl = StringUtils::splitString(path, '/');
            auto pc = root->resolveContainerForPath(pathSpl);
            if(pc){
                savePreset(dynamic_pointer_cast<ParameterContainer>(pc), name);
            }
            else{
                DBG("[preset] no container found for " << path << " and " << name);
            }
        });
        addParameter<ActionParameter>("recall", [this](const string & s) {
            // if (!root) {DBGE("preset root not set"); return;}
            auto it = s.find(' ');
            auto path = StringUtils::trim(s.substr(0, it));
            auto name = StringUtils::trim(s.substr(it + 1));
            auto pathSpl = StringUtils::splitString(path,'/');
            auto pc = root->resolveContainerForPath(pathSpl);
            if(pc){
                recallPreset(dynamic_pointer_cast<ParameterContainer>(pc), name);
            }
            else{
                DBG("[preset]  no container found for " << path << " and " << name);
            }
        });


    }
    void setup(const string & _baseDirectory){
        baseDirectory = _baseDirectory;
        if(baseDirectory[baseDirectory.size()-1]!='/'){baseDirectory+='/';}
        DBG("prset will be saved in " << baseDirectory);
    }

    string getFilePathForContainer(ParameterContainer::Ptr pc) {
        auto s = baseDirectory+StringUtils::joinString(pc->getCurrentPath(),'_'); 
        if(s[s.size()-1]=='/' || s[s.size()-1]==' ')s = s.substr(0,s.size()-1);
        return s;
    }
    Node::NodeView getPresetNodeView(ParameterContainer::Ptr pc) {
        return pc->createNodeView<ParameterBase>(
                                             [](ParameterBase * c) {
                                                 return c->isSavable;
                                             },
                                             [this](Node * n) {
                                                 if (auto en = n->leaves.getWithTypeNamed<Parameter<bool>>("enabled")) {return en->getValue();}
                                                 return true;
                                             });

    }
    void savePreset(ParameterContainer::Ptr pc, const string & name) {
        auto pathStr = getFilePathForContainer(pc);
        pathStr += name + ".preset";
        DBG("saving preset to " << pathStr);
        ofstream f;
        f.open(pathStr);
        if (!f.is_open()) {f.close(); DBGE("can't write preset at " << pathStr); return;}
        auto nv = getPresetNodeView(pc);
        f << nv->toNiceString();
        f.close();
    }

    void recallPreset(ParameterContainer::Ptr pc, const string & name) {
        auto pathStr = getFilePathForContainer(pc);
        pathStr += name + ".preset";
        DBG("loading preset from " << pathStr);
        ifstream f;
        f.open(pathStr);
        if (!f.is_open()) {f.close(); DBGE("no preset found for " << pathStr); return;}
        std::string s(std::istreambuf_iterator<char>(f), {});
        pc->setStateFromString(s);
        f.close();
    }
    
private:
    ParameterContainer::Ptr  root;
    string baseDirectory;
};
