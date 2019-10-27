// utility class inspired from boost::any
//https://www.artima.com/cppsource/type_erasure2.html
#include <map>
#include "StringUtils.h"
#include <memory>

using std::string;
using std::map;
using std::shared_ptr;

#if 1



// generic container
template <class LeafType>
class TypedContainer{
public:
        typedef map<string,LeafType> LeafMapType;
        typedef TypedContainer<LeafType> ContainerType;
        typedef map<string,ContainerType> ContainerMapType;
        typedef shared_ptr<ContainerType > Ptr;



        template < class T>
        void addChild(const string & name,const T& v){leaves.emplace(name,LeafType(v));}//.first->second;}
        template <class T>
        void addContainer(const string & name){containers.emplace(name,T());}

        template <class T,class L = LeafType> 
        T reduceChilds(std::function<void(T&,const L &)> fun,T state={}){
                for(const auto & l:leaves){fun(state,l.second);}
                return state;

        }
protected :
        LeafMapType leaves;
        ContainerMapType containers;
};



/////////
// type erasure

class PlaceHolder{
  public:
  virtual ~PlaceHolder() {}
  virtual PlaceHolder* clone() const=0;
  virtual string toString() =0;
  virtual void fromString(const string & s) = 0;

  template<class T>
  const T * castBase()const{return dynamic_cast<const T*>((T*)getRawPtr());}

protected:
  virtual const void* getRawPtr()const = 0;
  
};

template<typename ValueType>
class Holder : public PlaceHolder
{
public:
  Holder(ValueType const & value) : held(value) {}
  virtual PlaceHolder* clone() const {return new Holder(held);}
  // Value type should define these
  string toString(){return held.toString();}
  void fromString(const string & s){held.fromString(s);}
  const void* getRawPtr()const final {return &held;}
private:
  ValueType held;
};

class Any{
public:

        // typedef sharedPtr<Any> Ptr;
  Any() : content(0) {}

template<class ValueType>
Any(ValueType const & value) : content(new Holder<ValueType>(value)) {}

Any(Any const & other) : content(other.content ? other.content->clone() : 0) {}

~Any() {delete content;}

// Value type should define these
  string toString(){return content->toString();}
  void fromString(const string & s){content->fromString(s);}

template<class T>
bool isType()const{return dynamic_cast<Holder<T>*>(content)!=nullptr;}


template<class T>
const T* getValueAs()const{if(auto h= dynamic_cast<Holder<T>*>(content)){return h.held;}return nullptr;}

template<class T>
const T* getValueAsBase()const{if(auto h= content->castBase<T>()){return h;}return nullptr;}

template<class T>
T& cast()const{return static_cast<Holder<T>&>(content);}

// Implement swap as swapping placeholder pointers, assignment
// as copy and swap.

private:
  PlaceHolder* content;
};



class DynamicObject : public TypedContainer<Any>{
public:
        
        string toString(int numSpace  = 0){
                string res = "{";
                int i = leaves.size()-1;
                for(auto & e:leaves){res+=e.first+":" + e.second.toString() + (i!=0?",":"");
                }
                i = containers.size()-1;
                for(auto & e:containers){res+=e.first+":" + static_cast<DynamicObject&>(e.second).toString()+ (i!=0?",":"");}
                res+="}";

                return toIndentedString(res,numSpace);
        }

        void fromString(const string & s){};
        bool isDynamicObject(const Any & a){return a.isType<DynamicObject>();}

};

#endif
