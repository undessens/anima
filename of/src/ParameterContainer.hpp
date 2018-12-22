#pragma once
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <iostream> //cout

#define DBG(x) std::cout <<"[verbose] : " << x << std::endl;
#define DBGW(x) std::cout <<"[warning] : " << x << std::endl;
#define DBGE(x) std::cerr <<"[error] : " << x << std::endl;
#include "StringUtils.h"
#include "MiscUtils.h"

using namespace std;
// using std::string;using std::shared_ptr;using std::map;using std::istream;using std::istringstream;using std::ostream;using std::ostringstream;using std::vector;using std::function;




// class LeafType;
class LeafParent {
    public :
    virtual ~LeafParent() {};
    // virtual void removeLeaf(LeafType * ) =0;

};

class LeafType {
public:
    LeafType(const string & n, LeafType * p = nullptr): name(n) {setParent(p);}
    virtual ~LeafType() {DBG("leaf deleting : " << getName());}
    virtual string stateToString()const = 0;
    virtual void stateFromString(const string &) = 0;
    const string &  getName() const {return name;};
    const LeafType * getParent()const {return parent;}
    void setParent(LeafType * p) {parent = p;}

    typedef vector<string> PathType;
    PathType getCurrentPath(LeafType* relativeTo=nullptr){
        PathType res{getName()};
        auto insp = getParent();
        while(insp && insp!=relativeTo && insp->getParent()!=nullptr){res.push_back(insp->getName());insp = insp->getParent();}
        return res;

    }
private:
    string name;
    LeafType * parent;
};


class Node : public LeafType , public LeafParent {
public:
    Node(const string & n, Node *parent = nullptr): LeafType(n, parent), nodes(this), leaves(this) {}
    virtual ~Node() {};
    typedef Node NodeType;


    template <class T>
    using PtrT =  shared_ptr<T>;

    typedef PtrT<NodeType> Ptr;
    typedef PtrT<LeafType> LeafPtr;


    template<class T>
    T * getParentAs() {return dynamic_cast<T*>(parent);}

    static const string & getRootName(){static string r("root");return r;}
    static const Node::Ptr getRoot(){
        return _sR();
    }
    static void setRoot(Node::Ptr r){
            _sR() = r;
        }
    static Node::Ptr & _sR(){
            static Node::Ptr root;
            return root;
        }
    Ptr resolveContainerForPath(const  PathType & path,Node::Ptr relativeTo=nullptr)const{
            auto insp = relativeTo;
            if(insp==nullptr && getParent()){insp =  ((Node*)getParent())->nodes.getByName(getName());}
            if(insp==nullptr){insp = getRoot();}
            if(path.size()==0){DBGE("no path provided");return nullptr;}
            for(auto &p:path){
                insp = insp->nodes.getByName(p);
                if(!insp){return nullptr;}
            }
            return insp;
    }
    LeafPtr resolveLeafForPath(PathType path,Node::Ptr relativeTo = nullptr)const{
        if(path.size()==0){DBGE("should provide leaf path");return nullptr;}
        auto leafName = path[path.size()-1];path.pop_back();
        Node::Ptr parentC = resolveContainerForPath(path,relativeTo);
        if(parentC){
            return parentC->leaves.getByName(leafName);
        }
        return nullptr;
    }

    template<class VT>
    class LeafContainer : public NamedPtrSet<VT> {
    public:
        typedef typename NamedPtrSet<VT>::Ptr Ptr;
        using NamedPtrSet<VT>::getContainer;

        LeafContainer(Node * _owner): listeners("leafContListeners") , owner(_owner) {};
        struct Listener: public ListenerBase {
            using ListenerBase::ListenerBase;
            virtual ~Listener() {};
            virtual void leafAdded(LeafContainer<VT> * ori, Ptr c) = 0;
            virtual void leafRemoved(LeafContainer<VT> * ori, Ptr c) = 0;
        };
        ListenerList<Listener> listeners;
        void addInternal(Ptr c) final          {c->setParent(owner); listeners.call(&Listener::leafAdded, this, c);}
        void removeInternal(Ptr c)final        {c->setParent(nullptr); listeners.call(&Listener::leafRemoved, this, c);}
        string toStringList()const             {int i = getContainer().size() - 1; string res ; for (const auto &v : getContainer()) {res += v.first + ":" + v.second->stateToString(); if (i != 0) {res += ",";} i--;} return res;}
        friend class Node;

    private:

        Node * owner;

    };

    typedef Node::Ptr NodeView;
    template<class T>
    NodeView createNodeView(function<bool(T*)> f, function<bool(NodeType*)> fnode = [](NodeType*) {return true;}, int maxRecursion = -1) {
        Node::Ptr res ( new Node(getName()) );
        for (auto &c : leaves.getContainer()) {
            if (auto cc = dynamic_pointer_cast<T>(c.second)) {
                if (f(cc.get())) {res->leaves.addShared(c.second);}
            }
        };
        if (maxRecursion != 0) {
            for (auto &c : nodes.getContainer()) {
                if (fnode(c.second.get())) {
                    auto vn = c.second->createNodeView<T>(f, fnode, maxRecursion - 1);
                    if (!vn->isEmpty()) {res->nodes.addShared(vn);}
                }
            }
        }
        return res;
    }


    void clearNode() {
        nodes.clear();
        leaves.clear();
    }

    bool isEmpty() {return nodes.size() == 0  && leaves.size() == 0;}



    string toNiceString()const {return StringUtils::indentJSON(stateToString(), 2);}
    // DynamicObject::Ptr createDynamicObject(){

    // }

    virtual string stateToString() const override {
        string res = "{";
        res += leaves.toStringList();
        if (leaves.size() > 0 && nodes.size() > 0 ) {res += ",";}
        res += nodes.toStringList();
        res += "}";
        return res;
    }

    virtual void stateFromString(const string & s) override {
        const string ignore = " \t\r\n";
        int depth = -1;
        string cs , name;
        for (const char c : s) {
            if (depth <= 0 && ignore.find(c) != string::npos)continue;
            if (c == '{' ) {depth++; if (depth == 0) { continue;}}
            if (c == '[') {depth++;}
            if (c == ']') {depth--;}
            if (c == ':' && depth == 0) {name = cs; cs = ""; continue;}
            if ((c == ',' && depth == 0) || (c == '}' && depth == 1) ) {
                if (c == '}') {cs += c; depth--;}
                if (auto cn = nodes.getByName(name)) {cn->stateFromString(cs);}
                else if (auto cn = leaves.getByName(name)) {cn->stateFromString(cs);}
                cs = ""; name = "";
                continue;
            }
            if (c == '}' ) {depth--; if (depth == -1)break;}
            if (depth >= 0) {cs += c;}
        }
        if (name != "" && cs != "") {
            if (auto cn = nodes.getByName(name)) {cn->stateFromString(cs);}
            else if (auto cn = leaves.getByName(name)) {cn->stateFromString(cs);}
        }

    }

    LeafContainer<Node> nodes;
    LeafContainer<LeafType> leaves;

protected:
    NodeType*  parent;

};


class ParameterBase : public LeafType, public WeakReferenceAble<ParameterBase> {
public:
    ParameterBase(const string & n): LeafType(n), isSavable(true), listeners("paramListeners") {}

    bool isSavable;
    struct Listener : public ListenerBase {
        using ListenerBase::ListenerBase;
        virtual ~Listener() {};
        virtual void valueChanged(ParameterBase *) = 0;
    };
    ListenerList<Listener> listeners;

};

class ParameterContainerFactory {
    public :
    typedef Node BaseObject;
    typedef std::function<BaseObject*()> constructorType;

    static ParameterContainerFactory & i() {static ParameterContainerFactory cf; return cf;}

    template<class T, class ...Args>
    void registerType(const string & typeName, Args... args) {
        auto existing = factory.find(typeName);
        if (existing != factory.end()) {
            // here we could check if updated version of class but well..
        }
        constructorType constr(createConstructor<T>(args...));
        DBG("type registered : " << typeName);
        factory[typeName] = constr;
        typeids[typeid(T).name()] = typeName;

    }
    BaseObject* generateFromType(const string & typeName) {return factory[typeName]();}

    // serialization
    template<class T>
    string instanceToString(T * ) {

    }

    template<class T>
    T * instanceFromString(const string & s) {
        auto  tn = typeids.find(typeid(T).name());
        if (tn != typeids.end()) {
            auto obj = dynamic_cast<T*>(generateFromType(tn->second));
            return obj;
        }
        return nullptr;
    }

private:

    template<class T, class ...Args>
    static constructorType createConstructor(Args... args) {
        return [args...]() {return new T(args...);};
    }

    map<string, constructorType> factory;
    map<string, string> typeids;

};


// Leaf Inmps
template<class T>
class Parameter  : public ParameterBase {
public:

    typedef Node::PtrT<Parameter<T>> Ptr;
    Parameter(const string & _name, const T & t): ParameterBase(_name), value(t), bhasChanged(false), isCommiting(false) {};
    // Parameter(const string & _name,const T && t):ParameterBase(_name),value(t){};
    const T & setValue(const T & t, Listener * from = nullptr) {
        bool isChange  = t != value;
        if (isChange)
            DBG("setting p:" << getName() << (isChange ? "change" : ""));
        bhasChanged |= isChange ;
        if (isChange){lastValue=value;value = t;}
        if (!isCommiting && isChange)notifyValueChanged(from);
        return value;
    }
    const T & getValue()const {return value;}
    bool hasChanged(bool clearFlag = true) {if (bhasChanged) {if (clearFlag) {bhasChanged = false;} return true;} return false;}
    const T & operator =(const T & t) {return setValue(t);}

    // const T& operator->()const{return getValue();}
    // explicit operator bool() const {return bool(getValue());};
    // explicit operator const T&()const{return getValue();}
    operator const T&() {return value;}

    void stateFromString(const string & s) override {setValue(StringUtils::ElemSerializer<T>::fromString(s));}
    string stateToString() const override {return StringUtils::ElemSerializer<T>::toString(value);}
    // virtual string toString()const{return stateToString();}
    // virtual void fromString(const string & s){return stateFromString(s);}
    void notifyValueChanged(Listener * from = nullptr) {
        if (!isCommiting) {
            // do stuff
            listeners.callExcluding(from, &Listener::valueChanged, this);
            // bhasChanged = false;
        }
    }

    struct CommitingSession {
        CommitingSession(Parameter<T> &_p): p(_p) {p.startCommit();}
        ~CommitingSession() {p.endCommit();}
        const Parameter<T> & p;
    };
    std::unique_ptr<CommitingSession> startCommitingSession() {return new CommitingSession(*this);};
    void startCommit() {isCommiting = true;}
    void endCommit() {isCommiting = false; if (bhasChanged) {notifyValueChanged();}}
    const T& getLastValue(){return lastValue;}
protected:
    T value;
    T lastValue;
private:


    bool bhasChanged;
    bool isCommiting;

};



// lazy param watcher not used atm
// template <class T>
// class ParamWatcher: public Parameter<T> {
//   ParamWatcher(const string & name , const T& toWatch ):
//     Parameter<T>(name, toWatch), watchedP(toWatch) {
//   }

//   bool notifyIfChanged()  {setValue(watchedP);}
// protected:
//   const T& watchedP;

// };

// template <class INNER, class OTHER>
// class ParamWatcherUnrelated: public Parameter<INNER> {
//   typedef std::function<void(const OTHER&, INNER&)>  SetterFType;
//   ParamWatcherUnrelated(const string & name , const OTHER& toWatch, SetterFType setterF):
//     setter(setterF),
//     watchedValue(setterF(toWatch)),
//     Parameter<INNER>(name, watchedValue),
//     watchedP(toWatch) {

//   }

//   bool notifyIfChanged() {
//     setter(watchedP, watchedValue);
//     setValue(watchedValue);
//   }

// protected:
//   INNER watchedValue;
//   const OTHER& watchedP;
//   const SetterFType setter;

// };


#define NUMERICOPERATORS(OTHERTYPE) \
void operator      =(const OTHERTYPE & o){setValue(o);}        \
const T  operator +(const OTHERTYPE & o)const{return getValue()+o;}        \
const T  operator -(const OTHERTYPE & o)const{return getValue()-o;}        \
const T  operator /(const OTHERTYPE & o)const{return getValue()/o;}        \
const T  operator *(const OTHERTYPE & o)const{return getValue()*o;}        \
void operator     +=(const OTHERTYPE & o){setValue(getValue()+o);}        \
void operator     -=(const OTHERTYPE & o){setValue(getValue()-o);}        \
void operator     *=(const OTHERTYPE & o){setValue(getValue()*o);}        \
void operator     /=(const OTHERTYPE & o){setValue(getValue()/o);}        \
bool operator     ==(const OTHERTYPE & o)const{return getValue()==o;}        \
bool operator     > (const OTHERTYPE & o)const{return getValue()>o;}        \
bool operator     >=(const OTHERTYPE & o)const{return getValue()>=o;}        \
bool operator     < (const OTHERTYPE & o)const{return getValue()<o;}        \
bool operator     <=(const OTHERTYPE & o)const{return getValue()<=o;}        \
bool operator     !=(const OTHERTYPE & o)const{return getValue()!=o;}        \

typedef Parameter<bool> BoolParameter ;
typedef Parameter<string> StringParameter ;

template <class T>
class NumericParameter : public Parameter<T> {
public:
    using Parameter<T>::Parameter; // inherit constructor
    using Parameter<T>::getValue;
    using Parameter<T>::setValue;

    const T & operator ++() {return setValue(getValue()++);}
    const T & operator --() {return setValue(getValue()--);}
    NUMERICOPERATORS(float)
    NUMERICOPERATORS(int)
    NUMERICOPERATORS(double)

};

typedef float floatParamType;
typedef NumericParameter<int> IntParameter;
typedef NumericParameter<floatParamType> FloatParameter;

template <class ValueType, class ElemType>
class IVecParameter : public Parameter<ValueType> {
public:
    using Parameter<ValueType>::Parameter;//(const string & name):Parameter<T>(name); // inherit constructor
    using Parameter<ValueType>::getValue;
    using Parameter<ValueType>::setValue;
    using Parameter<ValueType>::value;
    using Parameter<ValueType>::notifyValueChanged;
    using Parameter<ValueType>::startCommit;
    using Parameter<ValueType>::endCommit;

    virtual bool canResize()const = 0;
    virtual int getSize()const = 0;
    virtual bool add(const ElemType & ) = 0;

    void setAt(const int i, const ElemType& v) {if (value[i] != v) {value[i] = v; notifyValueChanged();}} // no bounds checks
    virtual const ElemType& operator[](const int i)const {return value[i];};

    string stateToString()const final {
        string res = "[";
        for (int i = 0 ; i < getSize() ; i++) {
            res += std::to_string(value[i]);
        }
        res += "]";
    }
    void stateFromString(const string & s) final{
        int depth = -1;
        string cs;
        int idx  = 0;
        startCommit();
        for (auto & c : s) {
            if (c == '[') {depth++; continue;}
            if ((c == ']' || c == ',') && depth == 0) {
                if (idx >= getSize()) {
                    if (!canResize()) {break;}
                    if (!add(StringUtils::ElemSerializer<ElemType>::fromString(cs))) {
                        // error while adding
                        DBG("cant add to vector");
                    }
                }
                else {
                    value[idx] = StringUtils::ElemSerializer<ElemType>::fromString(cs);
                }
                if (c == ',') {idx++; continue;}
                else {break;}
            }
            cs += c;
        }
        endCommit();

    }
};

template <class T, int DIM>
class FixedSizeVecParameter : public IVecParameter<T[DIM], T> {
public:
    using IVecParameter<T[DIM], T>::IVecParameter;
    bool canResize()const final {return false;}
    int getSize()const final {return DIM;};
    bool  add(const T & ) final {return false;};
};

class Vec2fParameter : public FixedSizeVecParameter<floatParamType, 2> {
public:
    using FixedSizeVecParameter<floatParamType, 2>::FixedSizeVecParameter;

    const floatParamType & getX()const {return value[0];}
    const floatParamType & getY()const {return value[1];}
    void  setX(const floatParamType & v) {setAt(0, v);}
    void  setY(const floatParamType & v) {setAt(1, v);}

};

class Vec3fParameter : public FixedSizeVecParameter<floatParamType, 3> {
public:
    using FixedSizeVecParameter<floatParamType, 3>::FixedSizeVecParameter;

    const floatParamType & getX()const {return value[0];}
    const floatParamType & getY()const {return value[1];}
    const floatParamType & getZ()const {return value[2];}
    void  setX(const floatParamType & v) {setAt(0, v);}
    void  setY(const floatParamType & v) {setAt(1, v);}
    void  setZ(const floatParamType & v) {setAt(2, v);}
};


template <class T>
class VecParameter : public IVecParameter<vector<T>, T> {
public:
    using IVecParameter<vector<T>, T>::IVecParameter;
    using IVecParameter<vector<T>, T>::value;
    bool canResize()const override {return true;}
    int getSize()const final {return value.size();};
    bool  add(const T & e) final { value.push_back(e); return true;};


};

class TriggerValueType {
public:
    void operator  =(const TriggerValueType & o) {};
    bool operator !=(const TriggerValueType & o)const {return true;}
    friend ostream& operator<<(ostream & os, const TriggerValueType& ) {return os;}
    friend istream& operator>>(istream & is, const TriggerValueType& ) {return is;}
};

class TriggerParameter : public Parameter<TriggerValueType> {
public:
    typedef Parameter<TriggerValueType> TriggerParameterType ;
    typedef std::function<void(TriggerParameter *)> TriggerFunctionType;
    TriggerParameter(const string & name , TriggerFunctionType f): TriggerParameterType(name, {}), tf(f) {isSavable = false;}
    string stateToString() const override {return "";}
    void stateFromString(const string &) override {};
    void trigger() {tf(this);}
private:
    TriggerFunctionType tf;
};

class ActionValueType: public string {
public:
//    void operator  =(const string & o) {string::operator=(o);};
//    operator string&(){return *this;}
    void operator  =(const ActionValueType & o) {};
    bool operator !=(const ActionValueType & o)const {return true;}
    
    friend ostream& operator<<(ostream & os, const ActionValueType& ) {return os;}
    friend istream& operator>>(istream & is, const ActionValueType& ) {return is;}
};

class ActionParameter : public Parameter<ActionValueType>, Parameter<ActionValueType>::Listener {
public:
    typedef Parameter<ActionValueType> ActionParameterType ;
    typedef std::function<void(const string & )> ActionFunctionType;
    ActionParameter(const string & name , ActionFunctionType f): ActionParameterType(name, {}), Parameter<ActionValueType>::Listener("ActionListener"), af(f) {isSavable = false;}
    // string stateToString() const override {return "";}
    // void stateFromString(const string &) override {};
    void valueChanged(ParameterBase *)final{executeAction(value);};
    void executeAction(const string & s) {af(s);}
private:
    ActionFunctionType af;
};

template<class PT>
class PCollection {
public:
    PCollection() {};
    typedef PT PType;
    typedef Node::PtrT<PType> ElemPtr;
    typedef NamedPtrSet<PType> CollectionType;
    // typedef Node::PtrT<PList<PType>> Ptr;
    void addToCollection(const ElemPtr p) {params.addShared(p);}

    template<class T>
    void setIfContains(const string & name, const T & v) {if (auto elem =  getRef(name)) {elem->setValue( v);}}

    ElemPtr getRef(const string & name) {return params.template getWithTypeNamedSafe<PType>(name);}
    void clear() {params.clear();}
    // auto begin(){return getCollection().begin();}
    // auto end(){return mapIterator().end();}

    const CollectionType & mapIterator()const {return params;}


    const typename CollectionType::ArrayView vIterator()const {return params.vIterator();}
private:
    CollectionType params;

};

template<class PT, class NodeOrLeafType>
class TypedView : protected Node::LeafContainer<NodeOrLeafType>::Listener {
public:
    typedef Node::LeafContainer<NodeOrLeafType> OriginContainerType;
    typedef Node::PtrT<PT> ElemPtr;
    TypedView(OriginContainerType & o, std::function<bool(PT*)> _filterF = [](PT*) {return true;}):
    Node::LeafContainer<NodeOrLeafType>::Listener("TypedView"),
    listeners("typedViewListeners"),
    originCont(o),
    filterF(_filterF){
        o.listeners.add(this);
        for (auto o : originCont.vIterator()) {leafAdded(&originCont, o);}
    }
    ~TypedView() {originCont.listeners.remove(this);}

    void leafAdded(OriginContainerType * ori, Node::PtrT<NodeOrLeafType> c) final{
        if (auto p = dynamic_pointer_cast<PT>(c)) {if (filterF(p.get())) {castedCont.addShared(p); listeners.call(&Listener::typedElementAdded, p);}}
    }
    void leafRemoved(OriginContainerType * ori, Node::PtrT<NodeOrLeafType> c) final{
        if (auto p = dynamic_pointer_cast<PT>(c)) {castedCont.remove(p); listeners.call(&Listener::typedElementRemoved, p);}
    }
    struct Listener: public ListenerBase {
        Listener(const string & n): ListenerBase(n) {};
        virtual ~Listener() {};
        virtual void typedElementAdded(const ElemPtr & ) = 0;
        virtual void typedElementRemoved(const ElemPtr &  ) = 0;
    };
    ListenerList<Listener> listeners;
    const NamedPtrSet<PT> & getNamedPtrSet()const {return castedCont;}

    size_t size()const {return originCont.size();}

    typename NamedPtrSet<PT>::Ptr operator[](int i) {
        int idx = 0; auto it = castedCont.begin();
        while ( !(i == idx || it == castedCont.end())) {it++;idx++;}
        if (it != castedCont.end()) {return it->second;}
        else {return typename NamedPtrSet<PT>::Ptr(nullptr);}
    }

private:

    OriginContainerType & originCont;
    NamedPtrSet<PT> castedCont;
    const std::function<bool(PT*)> filterF;
};



template<class PT>
class PList : public Node {
public:

    using Node::Node;

    typedef PT PType;
    typedef Node::PtrT<PType> ElemPtr;
    typedef Node::PtrT<PList<PType>> Ptr;

    template<class T>
    void setIfContains(const string & name, const T & v) {if (auto elem =  getRef(name)) {elem->setValue( v);}}

    ElemPtr getRef(const string & name) {return leaves.getWithTypeNamedSafe<PType>(name);}

    template<class T>
    ElemPtr addParam(const string & n, const T & v) {return leaves.createWithType<PType>(n, v);}


};







class ParameterContainer : public Node, private Node::LeafContainer<LeafType>::Listener, public WeakReferenceAble<ParameterContainer>, protected ParameterBase::Listener {
public:
    ParameterContainer(const string & n): Node(n), Node::LeafContainer<LeafType>::Listener(n), ParameterBase::Listener(n) {
        leaves.listeners.add(this);
    };
    using Node::PtrT;
    typedef PtrT<ParameterContainer> Ptr;
    
    template <class T, class ...Args>
    PtrT<T> addParameter(const string & n, Args... args) {
        auto p = leaves.createWithType<T>(n, args...);
        return p;
    }
    
    template <class ...Args>
    PtrT<TriggerParameter> addTrigger(const string & n, Args... args) {
        return leaves.createWithType<TriggerParameter>(n, args...);
    }
    
    template <class T = ParameterContainer, class ...Args>
    PtrT<T> addParameterContainer(const string & n, Args... args) {
        return nodes.createWithType<T>(n, args...);
    }
    
    void addSharedParameterContainer(Ptr other) {
        nodes.addShared(other);
    }
    
    virtual void parameterValueChanged(ParameterBase * p) {};
    
private:
    void valueChanged(ParameterBase * p) {parameterValueChanged(p);}
    
    
    void leafAdded(LeafContainer<LeafType> * ori, LeafPtr c) final{
        // DBG("param container : adding p : " << c->getName());
        // if(ori==&leaves){
        if (auto p = dynamic_cast<ParameterBase*>(c.get())) {p->listeners.add(this);}
        // }
    }
    void leafRemoved(LeafContainer<LeafType> * ori, LeafPtr c) final{
        // DBG("param container : removing p : " << c->getName());
        // if(ori==&leaves){
        if (auto p = dynamic_cast<ParameterBase*>(c.get())) {p->listeners.remove(this);}
        // }
    }
};

