#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out VertexData
{
    int id;
    vec3 normal;
} vertex;

void main()
{

	gl_Position = vec4(position, 1);

	vertex.id = gl_VertexID;
	vertex.normal = normal;

}