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
  aiColor3D pOut{0};
  aiReturn success = material->Get(AI_MATKEY_COLOR_DIFFUSE, pOut);
  if (success != aiReturn_SUCCESS) {
    std::cerr << "Missing diffuse color for material." << std::endl;
    return;
  }
  for (uint i = 0; i <= 2; i++) {
    mDiffuse[i] = pOut[i];
  }
};
const glm::vec3 &Material::getDiffuse() const { return mDiffuse; }
