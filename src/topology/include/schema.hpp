#ifndef SCHEMA_HPP_
#define SCHEMA_HPP_

#include "types.hpp"
#include "utils.hpp"

namespace prometheus::topology {

    class Vertex;
    class Edge;
    class Fin;
    class Loop;
    class Face;
    class Shell;
    class Region;
    class Body;
    class Modeller;


    class Vertex {

        public: 

        private: 

        Body* body_;
        // UID of node
        Handle id_;
        // owning body
        Handle body;
        // Point of vertex
        Handle point_;
        // Head of singly-linked chain of fins referencing this vertex
        Handle fin_;
    };

    class Edge {
        public: 


        private: 

        Body* body_;
        // UID of node
        Handle id_;
	    // Head of singly-linked chain of fins around edge
        Handle fin_;
        // Underlying curve
        Handle curve_;
        // Orientation relative to curve
        bool sense_;
    };

    class Fin {

        public: 


        private: 

        Body* body_;
        // UID of node
        Handle id_;
        // Loop of fin
        Handle loop_;
        // Prev fin around loop
        Handle fin_prev_;
        // Next fin around loop
        Handle fin_next_;
        // Forward vertex of fin
        Handle vertex_;
        // Next fin around edge (clockwise looking along edge)
        Handle fin_next_on_edge_;
        // Edge of the fin
        Handle edge_;
        // Next fin in list referencing `vertex_`
        Handle fin_next_at_vertex_;
        // Orientation relative to to the edge
        bool sense_;
    };

    class Loop {

        public: 

        private: 

        Body* body_;
        // UID of node
        Handle id_;
        // Arbitrary fin in loop
        Handle fin_;
        // Next loop in face
        Handle loop_next_in_face_;
        // Owning face
        Handle face_;
    };

    class Face {

        public: 

        private: 

        Body* body_;
        // UID of node
        Handle id_;
        // Previous back-face of shell
        Handle face_prev_in_back_shell_;
        // Next back-face of shell
        Handle face_next_in_back_shell_;
        // Previous front-face of shell
        Handle face_prev_in_front_shell_;
        // Next front-face of shell
        Handle face_next_in_front_shell_;
        // Shell for which this is a back-face
        Handle shell_back_;
        // Shell for which this is a front-face
        Handle shell_front_;
        // Previous face on surface
        Handle face_prev_on_surface_;
        // Next face on surface
        Handle face_next_on_surface_;
        // Surface
        Handle surface_;
        // Orientation w.r.t surface
        bool sense_;

    };

    class Shell {

        public: 

        private: 

        Body* body_;
        // UID of node
        Handle id_;
        // Prev Shell in Region
        Handle shell_prev_in_region_;
        // Next Shell in Region 
        Handle shell_next_in_region_;
        // Head of linked-list of back faces
        Handle faces_back_;
        // Head of linked-list of front-faces
        Handle faces_front_;
        // Region of shell
        Handle region_;
    };


    enum class RegionType {
        Solid, Void
    };

    class Region {

        Body* body_;
        // UID of node
        Handle id_;
        // Head of linked-list of shells
        Handle shell_;
        // Solid or void
        RegionType type_;
        
    };


    enum class BodyType {
        ManifoldSolid, ManifoldSheet, ManifoldWire, NonManifold
    };

    class Body {

        public: 

        private: 

        SlotPool<Vertex> vertices_;
        SlotPool<Edge> edges_;
        SlotPool<Fin> fins_;
        SlotPool<Loop> loops_;
        SlotPool<Face> faces_;
        SlotPool<Shell> shells_;
        SlotPool<Region> regions_;
    };


    class Modeller {
        public: 

        private: 

        SlotPool<Body> bodies;
    };



}


#endif
