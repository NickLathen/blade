#version 300 es
precision highp float;

in vec4 color;
in vec3 normal;
out vec4 FragColor;
void main() {
  float ambient = .1;
  vec3 lightDir = normalize(vec3(1,1,1));
  float diffuse = max(dot(normal, lightDir), 0.0);
  FragColor = (ambient + diffuse) * color;
};
