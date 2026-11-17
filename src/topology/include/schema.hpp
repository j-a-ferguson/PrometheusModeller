#ifndef SCHEMA_HPP_
#define SCHEMA_HPP_

#include "types.hpp"

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
        NodeId id_;
        // owning body
        NodeId body;
        // Point of vertex
        NodeId point_;
        // Head of singly-linked chain of fins referencing this vertex
        NodeId fin_;
    };

    class Edge {
        public: 


        private: 

        Body* body_;
        // UID of node
        NodeId id_;
	    // Head of singly-linked chain of fins around edge
        NodeId fin_;
        // Underlying curve
        NodeId curve_;
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
        NodeId loop_;
        // Prev fin around loop
        NodeId fin_prev_;
        // Next fin around loop
        NodeId fin_next_;
        // Forward vertex of fin
        NodeId vertex_;
        // Next fin around edge (clockwise looking along edge)
        NodeId fin_next_on_edge_;
        // Edge of the fin
        NodeId edge_;
        // Next fin in list referencing `vertex_`
        NodeId fin_next_at_vertex_;
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
        NodeId fin_;
        // Next loop in face
        NodeId loop_next_in_face_;
        // Owning face
        NodeId face_;
    };

    class Face {

        public: 

        private: 

        Body* body_;
        // UID of node
        NodeId id_;
        // Previous back-face of shell
        NodeId face_prev_in_back_shell_;
        // Next back-face of shell
        NodeId face_next_in_back_shell_;
        // Previous front-face of shell
        NodeId face_prev_in_front_shell_;
        // Next front-face of shell
        NodeId face_next_in_front_shell_;
        // Shell for which this is a back-face
        NodeId shell_back_;
        // Shell for which this is a front-face
        NodeId shell_front_;
        // Previous face on surface
        NodeId face_prev_on_surface_;
        // Next face on surface
        NodeId face_next_on_surface_;
        // Surface
        NodeId surface_;
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
        NodeId shell_prev_in_region_;
        // Next Shell in Region 
        NodeId shell_next_in_region_;
        // Head of linked-list of back faces
        NodeId faces_back_;
        // Head of linked-list of front-faces
        NodeId faces_front_;
        // Region of shell
        NodeId region_;
    };


    class Region {

        Body* body_;
        // UID of node
        NodeId id_;
        // Head of linked-list of shells
        NodeId shell_;
        // 
    };



}


#endif
