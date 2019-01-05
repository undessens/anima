#pragma once
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <iostream> //cout
#include <csignal> // TRIGGER_BREAK

//#define TRIGGER_BREAK std::raise(SIGINT);
#define DBG(x) std::cout <<"[verbose] : " << x << std::endl;
#define DBGW(x) std::cout <<"[warning] : " << x << std::endl;
#define DBGE(x) {std::cerr <<"[error] : " << x << std::endl;}
//#define passert(x) {if(!(x)){TRIGGER_BREAK}}
#include "StringUtils.h"
#include "MiscUtils.h"

using namespace std;
// using std::string;using std::shared_ptr;using std::map;using std::istream;using std::istringstream;using std::ostream;using std::ostringstream;using std::vector;using std::function;




class LeafType {
public:
    LeafType(const string & n, LeafType * p = nullptr): name(n) , parent(p) {}
    virtual ~LeafType() {DBG("leaf deleting : " << getName());}
    virtual string stateToString()const = 0;
    virtual void setStateFromString(const string &) = 0;
    const string &  getName() const {return name;};
    const LeafType * getParent()const {return parent;}
    void setParent(LeafType * p) {
        parent = p;
    }

    typedef vector<string> PathType;
    PathType getCurrentPath(LeafType* relativeTo = nullptr) {
        PathType res{name};
        LeafType * insp = LeafType::parent;
        while (insp != nullptr && insp != relativeTo && insp->parent != nullptr) {
            res.push_back(insp->name);
            insp = insp->parent;
        }
        std::reverse(res.begin(), res.end());
        return res;

    }
private:
    string name;
    LeafType * parent;
};


class Node : public LeafType {
public:
    Node(const string & n, Node *parent = nullptr, bool _shouldOwn = true): LeafType(n, parent), nodes(this), leaves(this), shouldOwn(_shouldOwn) {}
    virtual ~Node() {};
    typedef Node NodeType;


    template <class T>
    using PtrT =  shared_ptr<T>;

    typedef PtrT<NodeType> Ptr;
    typedef PtrT<LeafType> LeafPtr;


    template<class T>
    const T * getParentAs() const {return dynamic_cast<T*>(LeafType::getParent());}

    static const string & getRootName() {static string r("root"); return r;}
    static const Node::Ptr getRoot() {
        return _sR();
    }
    static void setRoot(Node::Ptr r) {
        _sR() = r;
    }
    static Node::Ptr & _sR() {
        static Node::Ptr root;
        return root;
    }
    Ptr resolveContainerForPath(const  PathType & path, Node::Ptr relativeTo = nullptr)const {
        auto insp = relativeTo;
        if (insp == nullptr && getParent()) {insp =  ((Node*)getParent())->nodes.getByName(getName());}
        if (insp == nullptr) {insp = getRoot();}
        if (path.size() == 0) {DBGE("no path provided"); return nullptr;}
        for (auto &p : path) {
            insp = insp->nodes.getByName(p);
            if (!insp) {return nullptr;}
        }
        return insp;
    }
    LeafPtr resolveLeafForPath(PathType path, Node::Ptr relativeTo = nullptr)const {
        if (path.size() == 0) {DBGE("should provide leaf path"); return nullptr;}
        auto leafName = path[path.size() - 1]; path.pop_back();
        Node::Ptr parentC = resolveContainerForPath(path, relativeTo);
        if (parentC) {
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
        void addInternal(Ptr c) final          {
            if (owner->shouldOwn) {c->setParent(owner);} listeners.call(&Listener::leafAdded, this, c);
        }
        void removeInternal(Ptr c)final        {
            if (owner->shouldOwn) {c->setParent(nullptr);} listeners.call(&Listener::leafRemoved, this, c);
        }
        string toStringList()const             {int i = getContainer().size() - 1; string res ; for (const auto &v : getContainer()) {res += v.first + ":" + v.second->stateToString(); if (i != 0) {res += ",";} i--;} return res;}
        friend class Node;

    private:

        Node * owner;

    };

    typedef Node::Ptr NodeView;
    template<class T>
    NodeView createNodeView(function<bool(T*)> f, function<bool(NodeType*)> fnode = [](NodeType*) {return true;}, int maxRecursion = -1) {
        Node::Ptr res ( new Node(getName(), nullptr, false) );
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


    void clearNode() {nodes.clear(); leaves.clear();}

    bool isEmpty() {return nodes.size() == 0  && leaves.size() == 0;}

    string toNiceString()const {return StringUtils::indentJSON(stateToString(), 2);}

    virtual string stateToString() const override {
        string res = "{";
        res += leaves.toStringList();
        if (leaves.size() > 0 && nodes.size() > 0 ) {res += ",";}
        res += nodes.toStringList();
        res += "}";
        return res;
    }

    virtual void setStateFromString(const string & s) override {
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
                if (auto cn = nodes.getByName(name)) {cn->setStateFromString(cs);}
                else if (auto cn = leaves.getByName(name)) {cn->setStateFromString(cs);}
                else {DBGE("no child found at : " << name);}
                cs = ""; name = "";
                continue;
            }
            if (c == '}' ) {depth--; if (depth == -1)break;}
            if (depth >= 0) {cs += c;}
        }
        if (name != "" && cs != "") {
            if (auto cn = nodes.getByName(name)) {cn->setStateFromString(cs);}
            else if (auto cn = leaves.getByName(name)) {cn->setStateFromString(cs);}
            else {DBGE("no child found at : " << name);}
        }

    }

    template<class T>
    void applyRecursively(std::function<void(T*)> f) {
        for (auto & l : leaves) {
            if (auto pl = dynamic_pointer_cast<T>(l.second)) {
                f(pl.get());
            }
        }
        for (auto & n : nodes) {n.second->applyRecursively<T>(f);}
    }

    LeafContainer<Node> nodes;
    LeafContainer<LeafType> leaves;

protected:
    const bool shouldOwn; // indicate if this is a normal container or just a reference to another
    // NodeType*  parent;

};


class ParameterBase : public LeafType, public WeakReferenceAble<ParameterBase> {
public:
    ParameterBase(const string & n): LeafType(n), isSavable(true), listeners("paramListeners"), bhasChanged(false), isCommiting(false) {}

    bool isSavable;
    struct Listener : public ListenerBase {
        using ListenerBase::ListenerBase;
        virtual ~Listener() {};
        virtual void valueChanged(ParameterBase *, void* notifier) = 0;
    };
    ListenerList<Listener> listeners;
    virtual void notifyValueChanged(void * from = nullptr) {
        if (!isCommiting) {
            // do stuff
            listeners.callExcluding(from, &Listener::valueChanged, this, from);
            // bhasChanged = false;
        }
    }



    struct CommitingSession {
        CommitingSession(ParameterBase *_p, void * _commiter): p(_p), commiter(_commiter) {p->startCommit();}
        ~CommitingSession() {p->endCommit(commiter);}
        ParameterBase * const p;
        void * commiter;
    };
    std::unique_ptr<CommitingSession> startScopedCommitingSession(void* commiter) {return make_unique<CommitingSession>(this, commiter);};
    void startCommit() {isCommiting = true;}
    void endCommit(void* commiter) {isCommiting = false; if (bhasChanged) {notifyValueChanged(commiter);}}



private :

    bool bhasChanged;
    bool isCommiting;
    template<class T>
    friend class Parameter;
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

    // // serialization
    // template<class T>
    // string instanceToString(T * ) {

    // }

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
    Parameter(const string & _name, const T & t): ParameterBase(_name), value(t) {};
    // Parameter(const string & _name,const T && t):ParameterBase(_name),value(t){};
    const T & setValue(const T & t, void * from = nullptr) {
        bool isChange  = t != value;
        // if (isChange)
        //     DBG("setting p:" << getName() << (isChange ? "change" : ""));
        bhasChanged |= isChange ;
        if (isChange) {lastValue = value; value = t;}
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

    void setStateFromString(const string & s) override {setValue(StringUtils::ElemSerializer<T>::fromString(s));}
    string stateToString() const override {return StringUtils::ElemSerializer<T>::toString(value);}
    // virtual string toString()const{return stateToString();}
    // virtual void fromString(const string & s){return setStateFromString(s);}

    const T& getLastValue() {return lastValue;}
protected:
    T value;
    T lastValue;
private:




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
        return res;
    }
    void setStateFromString(const string & s) final{
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
    void setStateFromString(const string &) override {};
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


class ActionParameter : public Parameter<ActionValueType>{
public:
    typedef Parameter<ActionValueType> ActionParameterType ;
    typedef std::function<void(const string & )> ActionFunctionType;
    ActionParameter(const string & name , ActionFunctionType f): ActionParameterType(name, {}), af(f) {isSavable = false;}
    // string stateToString() const override {return "";}
    // void setStateFromString(const string &) override {};
    void notifyValueChanged(void * from = nullptr)final{
        executeAction(value);
        Parameter<ActionValueType>::notifyValueChanged(from);
    };
    
    void executeAction(const string & s) {af(s);}
private:
    ActionFunctionType af;
};

template <class T>
class TypedActionParameter : public Parameter<T> {
public :
    typedef std::function<void(const T & )> ActionFunctionType;
    TypedActionParameter(const string & name, const T & defaultV, ActionFunctionType f): Parameter<T>(name, defaultV), af(f) {

    }
    void notifyValueChanged(void * from = nullptr)final{
        af(Parameter<T>::value);
        Parameter<T>::notifyValueChanged(from);
    }
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
        filterF(_filterF) {
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
        while ( !(i == idx || it == castedCont.end())) {it++; idx++;}
        if (it != castedCont.end()) {return it->second;}
        else {return typename NamedPtrSet<PT>::Ptr(nullptr);}
    }

    typename NamedPtrSet<PT>::Ptr operator[](const string & name) {return castedCont.getByName(name);}

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
    ParameterContainer(const string & n): Node(n),
        Node::LeafContainer<LeafType>::Listener(n),
        ParameterBase::Listener(n),
        parameterContainerListeners("containerListeners") {
        leaves.listeners.add(this);
    };
    using Node::PtrT;
    typedef PtrT<ParameterContainer> Ptr;

    const ParameterContainer *  getParent()const {return Node::getParentAs<const ParameterContainer>();}

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

    Ptr addSharedParameterContainer(Ptr other) {
        nodes.addShared(other);
        return other;
    }

    virtual void parameterValueChanged(ParameterBase * p) {};


    struct Listener : public ListenerBase {
        Listener(const string & n): ListenerBase(n) {};
        virtual ~Listener() {};

        virtual void childParameterChanged(ParameterContainer* parent, ParameterBase * p) = 0;

    };
    ListenerList<Listener> parameterContainerListeners;

private:
    void valueChanged(ParameterBase * p, void* notifier) {
        parameterValueChanged(p);

        parameterContainerListeners.callExcluding(notifier, &Listener::childParameterChanged, this, p);
        auto insp = ParameterContainer::getParent();
        while (insp) {insp->parameterContainerListeners.callExcluding(notifier, &Listener::childParameterChanged, this, p); insp = insp->getParent();}

    }


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

