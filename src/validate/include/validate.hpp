#ifndef VALIDATE_HPP_
#define VALIDATE_HPP_

#include "schema.hpp"

namespace prometheus::validate {

using namespace topology;
using namespace geometry;

struct Result {
    bool ok;
};

Result validate(Handle<Body> body);



}


#endif 