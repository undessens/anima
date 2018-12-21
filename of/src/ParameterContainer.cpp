// #include "AppConfig.h"

// #include "ParameterContainer.hpp"


// std::ostream& operator<<(std::ostream& os, const Parameter<string>& _v){auto v = _v.value;os <<"\"";for(const auto & e:v){if( e=='\"'){os << '\\';}os << e;}os << '\"'; return os;}
// std::istream& operator>>(std::istream& is,Parameter<string>& _v){
//         auto v = _v.value;
//       char c;
//       bool isEscaped = false;
//       while(is >> c){
//             if(c=='\\'){isEscaped = true; continue;}
//             if(!isEscaped && c=='\"'){break;}
//             v+=c;
//             isEscaped=false;
//             }return is;
//     }






