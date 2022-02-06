//
// Created by Magnus Nordlander on 2021-06-26.
//

#include "polymath.h"

double polynomial4(double a, double b, double c, double d, double x) {
    // y = ax^3 + bx^2 + cx + d
    double squaredX = x*x;
    double cubedX = squaredX*x;

    return (double)(a*cubedX + b*squaredX + c*x + d);
}