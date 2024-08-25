#include "Material.hpp"
#include <iostream>

// Material Material.002
// ?mat.name
// $mat.shadingm 3
// $mat.illum 2
// $clr.ambient 1.00, 1.00, 1.00
// $clr.diffuse 0.02, 0.80, 0.00
// $clr.specular 0.50, 0.50, 0.50
// $clr.emissive 0.00, 0.00, 0.00
// $mat.shininess 250.00
// $mat.opacity 1.00
// $clr.transparent 1.00, 1.00, 1.00
// $mat.anisotropyFactor 0.00
// $mat.refracti 1.50

Material::Material(const aiMaterial *material) {
  aiColor3D colorOut;
  float shininessOut;
  if (material->Get(AI_MATKEY_COLOR_AMBIENT, colorOut) != aiReturn_SUCCESS) {
    std::cerr << "Missing ambient color for material." << std::endl;
  } else {
    memcpy(&mProperties.ambientColor, &colorOut.r, sizeof(float) * 3);
  }
  if (material->Get(AI_MATKEY_COLOR_DIFFUSE, colorOut) != aiReturn_SUCCESS) {
    std::cerr << "Missing diffuse color for material." << std::endl;
  } else {
    memcpy(&mProperties.diffuseColor, &colorOut.r, sizeof(float) * 3);
  }
  if (material->Get(AI_MATKEY_COLOR_SPECULAR, colorOut) != aiReturn_SUCCESS) {
    std::cerr << "Missing specular color for material." << std::endl;
  } else {
    memcpy(&mProperties.specularColor, &colorOut.r, sizeof(float) * 3);
  }
  if (material->Get(AI_MATKEY_SHININESS, shininessOut) != aiReturn_SUCCESS) {
    std::cerr << "Missing shininess for material." << std::endl;
  } else {
    mProperties.shininess = shininessOut;
  }
};
const BSDFMaterial &Material::getProperties() const { return mProperties; };
