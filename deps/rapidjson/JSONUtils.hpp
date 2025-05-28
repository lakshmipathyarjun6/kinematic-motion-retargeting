#ifndef JSON_UTILS_hpp_
#define JSON_UTILS_hpp_

#include <iostream>

#include "document.h"

using namespace rapidjson;
using namespace std;

Document loadJSON(string inputFile);
void writeJSON(Document &d, string outputFile);

#endif // JSON_UTILS_hpp_
