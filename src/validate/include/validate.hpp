#ifndef VALIDATE_HPP_
#define VALIDATE_HPP_

#include "schema.hpp"
#include "../src/validate_internal.hpp"

namespace prometheus::validate {

using namespace topology;
using namespace geometry;

using internal::Result;
using internal::ErrorInfo;
using internal::ErrorCode;
Result validate(Handle<Body> body);

}


#endif 