
#include "ParameterContainer.hpp"
#include "ofMain.h"
template<>
string Parameter<ofVec2f>::stateToString() const {return "[" + ofToString(value) + "]"; }

template<>
string Parameter<ofVec3f>::stateToString() const {return "[" + ofToString(value) + "]"; }

template<>
void Parameter<ofVec3f>::stateFromString(const string & s) {
  if (s == "")return;
  int start = s.find('[');
  int end = s.find(']');
  if (start == string::npos || end == string::npos || end == start) {return;}
  auto sub = s.substr(start, end - start);
  setValue()

}