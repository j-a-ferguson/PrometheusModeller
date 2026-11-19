#include "validate.hpp"



namespace prometheus::validate {

    namespace {
        
        Result validate_manifold_solid(Body& body) {
            Result res;




            return res;
        }
        Result validate_manifold_sheet(Body& body) {
            Result res;
            return res;
        }
        Result validate_manifold_wire(Body& body) {
            Result res;
            return res;
        }
        Result validate_manifold_general(Body& body) {
            Result res;
            return res;
        }
    }


    Result validate(Handle<Body> body) {
        Body& b = body.get();
        switch(b.type()) {
            case BodyType::ManifoldSolid: {
                return validate_manifold_solid(b);
            }
            break;
            case BodyType::ManifoldSheet: {
                return validate_manifold_sheet(b);
            }
            break;
            case BodyType::ManifoldWire: {
                return validate_manifold_wire(b);
            }
            break;
            case BodyType::General: {
                return validate_manifold_general(b);
            }
            break;
        }
    }

}