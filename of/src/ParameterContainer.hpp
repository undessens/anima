#pragma once
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <iostream> //cout

#include "StringUtils.h"

using namespace std;
// using std::string;using std::shared_ptr;using std::map;using std::istream;using std::istringstream;using std::ostream;using std::ostringstream;using std::vector;using std::function;

#define DBG(x) std::cout <<"DBG : " << x << std::endl;


// class LeafType;
class LeafParent {
public :
  virtual ~LeafParent() {};
  // virtual void removeLeaf(LeafType * ) =0;

};

class LeafType {
public:
  LeafType(const string & n, LeafParent * p = nullptr): name(n) {setParent(p);}
  virtual ~LeafType() {}
  virtual string stateToString()const = 0;
  virtual void stateFromString(const string &) = 0;
  const string &  getName() {return name;};
  void setParent(LeafParent * p) {parent = p;}
  const LeafParent * getParent()const {return parent;}

private:
  string name;
  LeafParent * parent;
};

template <class VT>
class NamedPtrSet : private map<string, shared_ptr<VT>> {
public:
  template <class T>
  using PtrT =  shared_ptr<T>;

  typedef PtrT<VT> Ptr;
  typedef map<string, Ptr> ContainerType;
  typedef typename ContainerType::const_iterator c_iterator;
  virtual void addInternal(const Ptr p ) {};
  virtual void removeInternal(const Ptr p ) {};
  c_iterator begin()const {return ContainerType::cbegin();}
  c_iterator end()const {return ContainerType::cend();}

  class ArrayView {
  public:
    ArrayView(ContainerType & owner): o(owner), m_begin(o.cbegin()), m_end(o.cend()) {};
    class ValueIterator {
    public:
      ValueIterator(const c_iterator & mapIt): it(mapIt) {}
      ValueIterator& operator=(const ValueIterator& other) {it = other.it; return *this;}
      ValueIterator& operator++() {it++; return *this;} //prefix increment
      Ptr operator*() const {return it->second;}
      bool operator!=(const ValueIterator& other)const {return it != other.it;}
    private:
      c_iterator  it;
      // friend void swap(iterator& lhs, iterator& rhs); //C++11 I think
    };
    ValueIterator begin()const {return ValueIterator(m_begin);}
    ValueIterator end()const {return ValueIterator(m_end);}
  private:
    const ContainerType & o;
    const c_iterator m_begin, m_end;
  };

  ArrayView getVectorIterator()const {return ArrayView(*(ContainerType*)this);}
  int size() const                                  {return getContainer().size();}
  void clear()                                      {ContainerType::clear();}
  const ContainerType &  getContainer()const        {return *this;}

  Ptr  getByName(const string & n)const             {auto ni = getContainer().find(n); if (ni != getContainer().end()) {return ni->second;}; return nullptr;}
  Ptr add(VT * c)                                   { return addShared( Ptr(c));}
  Ptr addShared(const Ptr ori)                      { addInternal(ori); ContainerType::operator[](ori->getName()) = ori; return ori;};
  bool remove(const Ptr p)                          {removeInternal(p); ContainerType::erase(p->getName());}


  // cast Helpers
  template<class T, class ...Args>
  PtrT<T> createWithType(const string & n, Args... args) {return static_pointer_cast<T>(add(new T(n, args...))) ; }

  template<class T>
  vector<PtrT<T>> getWithType()const                     {vector<PtrT<T>> res ; for (auto &c : getContainer()) {if (auto cc = dynamic_pointer_cast<T>(c.second)) {res.push_back(cc);}}; return res;}

  template<class T>
  vector<PtrT<T>> getWithTypeSafe()const                {vector<PtrT<T>> res(getContainer().size()); int i = 0 ; for (auto &c : getContainer()) {res[i] = static_pointer_cast<T>(c.second); i++;}; return res;}

  template<class T>
  PtrT<T> getWithTypeNamed(const string & n)const       {return dynamic_pointer_cast<T>(getByName(n));}

  template<class T>
  PtrT<T> getWithTypeNamedSafe(const string & n)const      {return static_pointer_cast<T>(getByName(n));}

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

  template<class VT>
  class LeafContainer : public NamedPtrSet<VT> {
    typedef typename NamedPtrSet<VT>::Ptr Ptr;
    using NamedPtrSet<VT>::getContainer;

    LeafContainer(Node * _owner): owner(_owner) {};
    void addInternal(Ptr c) final                                  {c->setParent(owner);}
    void removeInternal(Ptr c)final {}
    string toStringList()const                              {int i = getContainer().size() - 1; string res ; for (const auto &v : getContainer()) {res += v.first + ":" + v.second->stateToString(); if (i != 0) {res += ",";} i--;} return res;}
    friend class Node;

  private:

    Node * owner;

  };

  template<class T>
  Node::Ptr createNodeView(function<bool(T*)> f, function<bool(NodeType*)> fnode = [](NodeType*) {return true;}, int maxRecursion = -1) {
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



  string toNiceString() {return StringUtils::indentJSON(stateToString(), 2);}
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


class ParameterBase : public LeafType {
public:
  ParameterBase(const string & n): LeafType(n), isSavable(true) {}

  bool isSavable;

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
  const T & setValue(const T & t) {
    bool isChange  = t != value;
    if (getName() != "resolution" && getName() != "mouse" && getName() != "time")
      DBG("setting p:" << getName() << (isChange ? "change" : ""));
    bhasChanged |= isChange ;
    value = t;
    if (!isCommiting && isChange)notifyValueChanged();
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
  void notifyValueChanged() {
    if (!isCommiting) {
      // do stuff

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
protected:
  T value;
private:


  bool bhasChanged;
  bool isCommiting;

};

// template<>
// string Parameter<string>::stateToString()const; {return value;};


// lazy param watcher
template <class T>
class ParamWatcher: public Parameter<T> {
  ParamWatcher(const string & name , const T& toWatch ):
    Parameter<T>(name, toWatch), watchedP(toWatch) {
  }

  bool notifyIfChanged()  {setValue(watchedP);}
protected:
  const T& watchedP;

};

template <class INNER, class OTHER>
class ParamWatcherUnrelated: public Parameter<INNER> {
  typedef std::function<void(const OTHER&, INNER&)>  SetterFType;
  ParamWatcherUnrelated(const string & name , const OTHER& toWatch, SetterFType setterF):
    setter(setterF),
    watchedValue(setterF(toWatch)),
    Parameter<INNER>(name, watchedValue),
    watchedP(toWatch) {

  }

  bool notifyIfChanged() {
    setter(watchedP, watchedValue);
    setValue(watchedValue);
  }

protected:
  INNER watchedValue;
  const OTHER& watchedP;
  const SetterFType setter;

};


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
  void operator      =(const TriggerValueType & o) {};
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

  const typename CollectionType::ArrayView vectorIterator()const {return params.getVectorIterator();}
private:
  CollectionType params;

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







class ParameterContainer : public Node {
public:
  using Node::Node;


  using Node::PtrT;
  typedef PtrT<ParameterContainer> Ptr;

  template <class T, class ...Args>
  PtrT<T> addParameter(const string & n, Args... args) {
    return leaves.createWithType<T>(n, args...);
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

};

