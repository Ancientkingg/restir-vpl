//
// Created by Rafayel Gardishyan on 24/04/2025.
//

#ifndef MATERIAL_H
#define MATERIAL_H
#include "color.h"

struct material {
    color c;
    float k_d = 1.0f; // Diffuse reflection coefficient
    float k_s = 0.0f; // Specular reflection coefficient
    float p = 1.0f; // Phong exponent
};

#endif //MATERIAL_H
