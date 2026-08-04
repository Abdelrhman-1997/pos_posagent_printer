#ifndef DSOFT_STATUSAPI_H
#define DSOFT_STATUSAPI_H

#include <string>

class DSoftStatusApi {
public:
  static std::string healthJson();
  static std::string printersJson();
};

#endif // DSOFT_STATUSAPI_H
