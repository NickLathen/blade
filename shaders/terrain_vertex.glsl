#version 300 es

uniform sampler2D uNoiseTexture;

uniform mat4 uMVP;
uniform mat4 uModelMatrix;
uniform mat4 uLightMVP;
uniform float uHeightScale;
uniform float uWidthScale;
uniform float uGridScale;
uniform int uResolution;

layout (location = 0) in uint aMaterialIdx;

out vec3 worldPos;
out vec2 texCoords;
out vec2 terrainCoords;
out vec4 lightSpacePosition;
flat out uint materialIdx;

vec3 GetVertexPosition(vec2 texCoords, float gridScale) {
  return vec3(
    (texCoords.x - 0.5f) * gridScale,
    0.0f,
    -(texCoords.y - 0.5f) * gridScale
  );
}

vec2 GetTexCoords(int i, int j, int resolution) {
  return vec2(
    float(j) / float(resolution),
    float(i) / float(resolution)
  );
}

void main() {
  //aPos, texCoords
  vec3 aPos;
  int rowWidth = uResolution * 2 + 2;
  int row = gl_VertexID / rowWidth;
  int rowVert = gl_VertexID % rowWidth;
  if (rowVert < rowWidth - 2) {
    texCoords = GetTexCoords(rowVert / 2, row + rowVert % 2, uResolution);
  } else if (rowVert == rowWidth - 2) {
    texCoords = GetTexCoords(uResolution - 1, row + 1, uResolution);
  } else if (rowVert == rowWidth - 1) {
    texCoords = GetTexCoords(0, row + 1, uResolution);
  }
  aPos = GetVertexPosition(texCoords, uGridScale);

  terrainCoords = texCoords * uWidthScale;
  float height = texture(uNoiseTexture, terrainCoords).r * uHeightScale;
  vec3 position = vec3(aPos.x, height, aPos.z);
  worldPos = (uModelMatrix * vec4(position, 1.0)).xyz;
  materialIdx = aMaterialIdx;
  gl_Position = uMVP * vec4(position, 1.0);
  lightSpacePosition = uLightMVP * vec4(position, 1.0);
}
 