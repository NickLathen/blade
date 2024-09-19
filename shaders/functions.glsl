float CalcShadowFactor(sampler2DShadow shadowMap, vec4 position, float bias) {
  float kShadowStrength = 0.8;
  vec3 ProjCoords = position.xyz / position.w;
  vec3 UVCoords;
  UVCoords.x = 0.5 * ProjCoords.x + 0.5;
  UVCoords.y = 0.5 * ProjCoords.y + 0.5;
  UVCoords.z = 0.5 * ProjCoords.z + 0.5;
  if (UVCoords.z < 0.0 || UVCoords.x < 0.0 || UVCoords.x > 1.0 || UVCoords.y < 0.0 || UVCoords.y > 1.0) {
    return 1.0;
  }
  UVCoords.z -= bias;
  float shadowFactor = 0.0;
  float texelSize = 1.0 / float(textureSize(shadowMap, 0));
  int nNeighbors = 2;
  float kernelSize = pow(float(nNeighbors) * 2.0 + 1.0, 2.0f);
  for (int y = -nNeighbors ; y <= nNeighbors ; y++) {
    for (int x = -nNeighbors ; x <= nNeighbors ; x++) {
      vec3 offset = vec3(float(x) * texelSize,
                          float(y) * texelSize,
                          0.0f);
      shadowFactor += texture(shadowMap, UVCoords + offset);
    }
  }
  return ((1.0 - kShadowStrength) + (shadowFactor * kShadowStrength) / kernelSize);
}
