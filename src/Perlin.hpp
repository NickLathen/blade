#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <vector>

// Fade function as defined by Ken Perlin. This eases coordinate values
// so that they will ease towards integral values. This smooths the final
// output.
float Fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }

// Linear interpolation function
float Lerp(float t, float a, float b) { return a + t * (b - a); }

// Hash function to generate gradient vectors based on input coordinates
int Hash(int x, int y, int seed) {
  int h = seed + x * 374761393 + y * 668265263; // Prime numbers
  h = (h ^ (h >> 13)) * 1274126177;
  return h ^ (h >> 16);
}

// Function to generate gradient vectors
float Gradient(int hash, float x, float y) {
  int h =
      hash & 3; // Convert low 2 bits of hash code into 4 gradient directions
  float u = h < 2 ? x : y;
  float v = h < 2 ? y : x;
  return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// Perlin noise function
float PerlinNoise(float x, float y, int seed) {
  int X = (int)floor(x) & 255;
  int Y = (int)floor(y) & 255;

  float xf = x - floor(x);
  float yf = y - floor(y);

  // Compute hash coordinates of the square corners
  int top_right = Hash(X + 1, Y + 1, seed);
  int top_left = Hash(X, Y + 1, seed);
  int bottom_right = Hash(X + 1, Y, seed);
  int bottom_left = Hash(X, Y, seed);

  // And add blended results from 4 corners of the square
  float u = Fade(xf);
  float v = Fade(yf);

  float n0, n1, n2, n3;
  n0 = Gradient(bottom_left, xf, yf);
  n1 = Gradient(bottom_right, xf - 1, yf);
  n2 = Gradient(top_left, xf, yf - 1);
  n3 = Gradient(top_right, xf - 1, yf - 1);

  // Blend the results
  float x1 = Lerp(u, n0, n1);
  float x2 = Lerp(u, n2, n3);
  float result = Lerp(v, x1, x2);

  // Return result normalized to the range [0, 1]
  return (result + 1.0f) / 2.0f;
}