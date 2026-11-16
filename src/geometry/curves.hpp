#ifndef CURVES_HPP_ 
#define CURVES_HPP_ 

#include <common.hpp>


namespace  prometheus::geometry {

class Curve 
{

    public: 
    virtual Vec3 eval(double t) = 0;
    virtual Vec3 diff(double t, int order) = 0;
    virtual Vec3 normal(double t, int order) = 0;
    virtual Vec3 tangent(double t, int order) = 0;
    virtual Vec3 binormal(double t, int order) = 0;
    virtual Vec3 jacobian(double t, int order) = 0;
};


class Line: public Curve
{

};

class Circle: public Curve
{

};

class Ellipse: public Curve 
{

};

class Hyperbola: public Curve 
{

};

}



#endif 