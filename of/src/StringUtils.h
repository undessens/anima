#pragma once
/// Helper
#include <string>
// #include <istream>
// #include <ostream>
#include <sstream>
namespace StringUtils {

using std::string;

static string indentJSON(const string & ins, int numSpaces) {
  if (numSpaces == 0) {return ins;}
  char c;
  string res;
  int i = 0, odepth = -1, adepth = -1;
  string spaceTab = "";
  while ((c = ins[i])) {
    if (c == '{') {
      res += c;
      odepth++;
      spaceTab = ""; for (int i = 0 ; i < numSpaces * (odepth + 1) ; i++) {spaceTab += " ";}
      res += "\n" + spaceTab;
    }
    else if (c == '}') {
      res += "\n";
      odepth--;
      spaceTab = ""; for (int i = 0 ; i < numSpaces * (odepth + 1) ; i++) {spaceTab += " ";}
      res += spaceTab + c;
    }
    else {
      res += c;
      if (c == '[') {adepth++;}
      else if (c == ']') {adepth--;}
      else if (c == ',' && adepth < 0) {res += "\n" + spaceTab;}
    }
    i++;
  }
  return res;

}


template <class T>
struct ElemSerializer{
  static T fromString(const string & s) {
    std::istringstream ss(s);
    T num;
    ss >> num;
    return num;
  }
  static string toString(const T & o) {
    std::ostringstream os;
    os << o;
    return os.str();
  }
};


}

template <class T>
std::ostream & operator<<(std::ostream & o, const std::vector<T> & v) {
  for (auto e : v) {
    o << "," << e;
  }
  return o;
}