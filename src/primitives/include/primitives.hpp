#ifndef PRIMITIVES_HPP_
#define PRIMITIVES_HPP_

#include "common.hpp"
#include "schema.hpp"
#include "slot_pool.hpp"


namespace prometheus::primitives {

using namespace topology;


struct CylinderForm{
    Vec3 bottom; 
    Vec3 axis; 
    double radius;
    double height;
};
Handle<Body> make_cyliner(Modeller& modeller, CylinderForm form);


}
#endif 