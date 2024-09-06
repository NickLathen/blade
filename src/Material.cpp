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
  aiColor3D color_out;
  float shininess_out;
  if (material->Get(AI_MATKEY_COLOR_AMBIENT, color_out) != aiReturn_SUCCESS) {
    std::cerr << "Missing ambient color for material." << std::endl;
  } else {
    memcpy(&m_properties.ambient_color, &color_out.r, sizeof(float) * 3);
  }
  if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color_out) != aiReturn_SUCCESS) {
    std::cerr << "Missing diffuse color for material." << std::endl;
  } else {
    memcpy(&m_properties.diffuse_color, &color_out.r, sizeof(float) * 3);
  }
  if (material->Get(AI_MATKEY_COLOR_SPECULAR, color_out) != aiReturn_SUCCESS) {
    std::cerr << "Missing specular color for material." << std::endl;
  } else {
    memcpy(&m_properties.specular_color, &color_out.r, sizeof(float) * 3);
  }
  if (material->Get(AI_MATKEY_SHININESS, shininess_out) != aiReturn_SUCCESS) {
    std::cerr << "Missing shininess for material." << std::endl;
  } else {
    m_properties.shininess = shininess_out;
  }
};
const BSDFMaterial &Material::GetProperties() const { return m_properties; };
