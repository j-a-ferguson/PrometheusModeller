#ifndef SCHEMA_HPP_
#define SCHEMA_HPP_

#include <memory>

#include "prometheus/common.hpp"
#include "types.hpp"
#include <prometheus/topology/utils.hpp>
#include "curves.hpp"
#include "surfaces.hpp"

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
    using Point = Vec3;
    using Curve = std::unique_ptr<geometry::Curve>;
    using Surface = std::unique_ptr<geometry::Surface>;


    class Vertex {

        public: 

        private: 

        Body* body_;
        // UID of node
        NodeId id_;
        // owning body
        Handle<Body> body;
        // Point of vertex
        Handle<Point> point_;
        // Head of singly-linked chain of fins referencing this vertex
        Handle<Fin> fin_;
    };

    class Edge {
        public: 


        private: 

        Body* body_;
        // UID of node
        NodeId id_;
	    // Head of singly-linked chain of fins around edge
        Handle<Fin> fin_;
        // Underlying curve
        Handle<Curve> curve_;
        // Orientation relative to curve
        bool sense_;
    };

    class Fin {

        public: 


        private: 

        Body* body_;
        // UID of node
        NodeId id_;
        // Loop of fin
        Handle<Loop> loop_;
        // Prev fin around loop
        Handle<Fin> fin_prev_;
        // Next fin around loop
        Handle<Fin> fin_next_;
        // Forward vertex of fin
        Handle<Vertex> vertex_;
        // Next fin around edge (clockwise looking along edge)
        Handle<Fin> fin_next_on_edge_;
        // Edge of the fin
        Handle<Edge> edge_;
        // Next fin in list referencing `vertex_`
        Handle<Fin> fin_next_at_vertex_;
        // Orientation relative to to the edge
        bool sense_;
    };

    class Loop {

        public: 

        private: 

        Body* body_;
        // UID of node
        NodeId id_;
        // Arbitrary fin in loop
        Handle<Fin> fin_;
        // Next loop in face
        Handle<Loop> loop_next_in_face_;
        // Owning face
        Handle<Face> face_;
    };

    class Face {

        public: 

        private: 

        Body* body_;
        // UID of node
        NodeId id_;
        // Previous back-face of shell
        Handle<Face> face_prev_in_back_shell_;
        // Next back-face of shell
        Handle<Face> face_next_in_back_shell_;
        // Previous front-face of shell
        Handle<Face> face_prev_in_front_shell_;
        // Next front-face of shell
        Handle<Face> face_next_in_front_shell_;
        // Shell for which this is a back-face
        Handle<Shell> shell_back_;
        // Shell for which this is a front-face
        Handle<Shell> shell_front_;
        // Previous face on surface
        Handle<Face> face_prev_on_surface_;
        // Next face on surface
        Handle<Face> face_next_on_surface_;
        // Surface
        Handle<Surface> surface_;
        // Orientation w.r.t surface
        bool sense_;

    };

    class Shell {

        public: 

        private: 

        Body* body_;
        // UID of node
        NodeId id_;
        // Prev Shell in Region
        Handle<Shell> shell_prev_in_region_;
        // Next Shell in Region 
        Handle<Shell> shell_next_in_region_;
        // Head of linked-list of back faces
        Handle<Face> faces_back_;
        // Head of linked-list of front-faces
        Handle<Face> faces_front_;
        // Region of shell
        Handle<Region> region_;
    };


    enum class RegionType {
        Solid, Void
    };

    class Region {

        Body* body_;
        // UID of node
        NodeId id_;
        // Head of linked-list of shells
        Handle<Shell> shell_;
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
