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


template <class T> //  can be further specialized if adding new param type
struct ElemSerializer {
  static T fromString(const string & s) {std::istringstream ss(s); T num; ss >> num; return num;}
  static string toString(const T & o) {std::ostringstream os; os << o; return os.str();}
};

static vector<string> splitString(string s, const char delim) {
  vector<string> res;
  size_t pos = 0;
  std::string token;
  while ((pos = s.find(delim)) != std::string::npos) {
    if (pos != 0) {
      token = s.substr(0, pos);
      res.push_back(token);
    }
    s.erase(0, pos + 1);
  }
  if (s != "") {res.push_back(s);}
  return res;
}

static string trim(string s) {
  char * c = &s[0];
  int start=0, end=0, i;
  i = 0;
  while (*c) {
    if (*c != ' ') {start = i; break;}
    c++; i++;
  }
  c = &s[s.size() - 1];
  i = s.size() - 1;
  while (c != &s[0]) {
    if (*c != ' ') {end = i; break;}
    c--; i--;
  }
  return s.substr(start, end - start + 1);
}
static string joinString(const vector<string>& splitted, const char delim) {
  string res;
  if (splitted.size() == 0) {return res;}
  auto i = splitted.begin();
  res += trim(*i);
  i++;
  while (i != splitted.end()) {
    if (res[res.size() - 1] != delim) {
      res += delim;
    }
    res += trim(*i);
    i++;
  }
  return res;
}

}

template <class T>
std::ostream & operator<<(std::ostream & o, const std::vector<T> & v) {
  for (auto e : v) {
    o << "," << e;
  }
  return o;
}
