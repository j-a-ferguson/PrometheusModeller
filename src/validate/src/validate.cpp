#include "validate.hpp"
#include "validate_internal.hpp"




namespace prometheus::validate {

    Result validate(Handle<Body> body) {
        Body& b = body.get();
        switch(b.type()) {
            case BodyType::ManifoldSolid: {
                return internal::validate_manifold_solid(b);
            }
            break;
            case BodyType::ManifoldSheet: {
                return internal::validate_manifold_sheet(b);
            }
            break;
            case BodyType::ManifoldWire: {
                return internal::validate_manifold_wire(b);
            }
            break;
            case BodyType::General: {
                return internal::validate_general(b);
            }
            break;
        }
    }

}