#pragma once

#include <map>
using std::map;
#include <vector>
using std::vector;
#include <unordered_set>
using std::unordered_set;

template <class VT>
class NamedPtrSet : private map<string, shared_ptr<VT>> {
public:
  template <class T>
  using PtrT =  shared_ptr<T>;
  virtual ~NamedPtrSet() {}

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
    size_t size() {return o.size();}
  private:
    const ContainerType & o;
    const c_iterator m_begin, m_end;
  };

  ArrayView vIterator()const {return ArrayView(*(ContainerType*)this);}
  size_t size() const                                  {return getContainer().size();}
  void clear()                                      {ContainerType::clear();}


  void apply(const std::function<void(VT*)> & f)const {for (auto &c : getContainer()) {f(c.second.get());}}


  template <class T>
  void clearElementsInMap(const T & cont) {
    for (auto c : cont) {
      if (contains(c.second)) {
        remove(c.second);
      }
    }
  }
  template <class T>
  void clearElementsInVec(const T & cont) {
    for (auto c : cont) {
      if (contains(c)) {
        remove(c);
      }
    }
  }
  const ContainerType &  getContainer()const        {return *this;}

  Ptr  getByName(const string & n)const             {
    auto ni = getContainer().find(n);
    if (ni != getContainer().end()) {
      return ni->second;
    };
    return nullptr;
  }
  Ptr operator[](const string & s) {return getByName(s);}

  Ptr add(VT * c)                                   { return addShared( Ptr(c));}
  Ptr addShared(const Ptr ori)                      { addInternal(ori); ContainerType::operator[](ori->getName()) = ori; return ori;};
  bool remove(const Ptr p)                          {if (contains(p)) {removeInternal(p); ContainerType::erase(p->getName()); return true;} return false;}
  bool contains(Ptr p)                              {return std::find_if(begin(), end(), [&p]( const std::pair<string, Ptr> & kv) {return (bool)(kv.second == p);}) != getContainer().end();}
  template<class T = VT>
  PtrT<T> findFirst(const std::function<bool(T*)> &func)const {for (auto & v : getWithType<T>()) {if (func(v.get())) {return v;}} return nullptr;}
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


template<class T>
class WeakReferenceAble {
public:

  typedef shared_ptr<T*> RefType;
  WeakReferenceAble() {v = make_shared<T*>(); *v = (T*)this;};
  virtual ~WeakReferenceAble() {clear();}
  RefType getRef()const {return v;}


private:
  void clear() {*v = nullptr;}
  RefType v;

};


template<class T>
class WeakReference {
public:

  WeakReference(T * other) {
    masterRef = static_pointer_cast<T*>(other->getRef());
  }

  T* get() const {return *masterRef;}

  struct Hasher
  {
    std::size_t operator()(const WeakReference<T>& k) const
    {
      using std::size_t;
      using std::hash;
      return hash<int>()(k.get());
    }
  };
private:
  typename WeakReferenceAble<T>::RefType masterRef;

};




class ListenerBase : public WeakReferenceAble<ListenerBase> {
public:
  ListenerBase(const string & n): name(n) {};
  virtual ~ListenerBase() {};
  const string name;
};

template<class T>
class ListenerList {
public:
  ListenerList(const string & n): name(n) {};
  void add(T*t) {remove(t); listeners.push_back(WeakReference<ListenerBase>(t));}
  void remove(T*t) {std::remove_if(listeners.begin(), listeners.end(), [t](const WeakReference<ListenerBase> &l) {return t == l.get();});}

  template<class FunType, class... Args>
  void call(FunType f, Args... args) {
    // DBG( "listener list: " <<name <<" " << listeners.size());
    bool shouldCleanUp = false;
    for (auto & l : listeners) {
      if (T * ll = (T*)l.get()) {std::bind(f, ll, args...)();}
      else {
        if (l.get()) {DBGE("listener list : wrong cast  ");}
        shouldCleanUp = true;
      }
    }
    if (shouldCleanUp) {
      cleanUp();
    }
  }
  template<class FunType, class... Args>
  void callExcluding(T* listenerToExclude, FunType f, Args... args) {
    // DBG( "listener list: " <<name <<" " << listeners.size());
    bool shouldCleanUp = false;
    for (auto & l : listeners) {
      if (listenerToExclude && l.get() == listenerToExclude) {continue;}
      if (T * ll = (T*)l.get()) {std::bind(f, ll, args...)();}
      else {
        if (l.get()) {DBGE("listener list : wrong cast  ");}
        shouldCleanUp = true;
      }
    }
    if (shouldCleanUp) {
      cleanUp();
    }
  }
  void cleanUp() {
    std::remove_if(listeners.begin(), listeners.end(), [](const WeakReference<ListenerBase> &l) {if (!l.get()) {DBGE("old listener still linked");} return !l.get();});
  }
private:
  vector<WeakReference<ListenerBase>> listeners;
  // unordered_set<WeakReference<T>, typename WeakReference<T>::Hasher> listeners;
  const string name;

};



