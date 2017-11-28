#version 150

//in vec4 col;
in vec2 tc;
//in vex3 n; //normals
out vec4 c;

uniform sampler2D textureImage; //textureImage has diffuse lighting properties built in
//uniform vec4 ambientLight; ambientLight / specularLight are uniform for each drawArrays call

void main()
{
  //vec3 lightDir = normalize(vec3(1,1,1));

  // compute the final pixel color
  //c = col;
  c = texture(textureImage, tc);// * clamp(dot(n, lightDir), 0, 1);
}

