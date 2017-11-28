#version 150

in vec3 position;
//in vec4 color;
//out vec4 col;
in vec2 texCoord;

out vec2 tc;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main()
{
  // compute the transformed and projected vertex position (into gl_Position) 
  // compute the vertex color (into col)
  gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
  //col = color;

  tc = texCoord;
}

