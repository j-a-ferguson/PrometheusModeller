#ifndef PRIMITIVES_HPP_
#define PRIMITIVES_HPP_

#include "common.hpp"
#include "json_formatter.hpp"
#include "schema.hpp"
#include "slot_pool.hpp"


namespace prometheus::primitives {

using namespace topology;
using namespace geometry;


struct CylinderForm{
    Vec3 bottom; 
    Vec3 axis; 
    double radius;
    double height;
};
void to_json(nlohmann::json &json, const CylinderForm &form);
Handle<Body> make_cyliner(Modeller& modeller, CylinderForm form);


} // namespace prometheus::primitives

template <>
struct std::formatter<prometheus::primitives::CylinderForm, char>
    : prometheus::JsonFormatter<prometheus::primitives::CylinderForm> {};

#endif
