#version 410 core

in vec3 in_Position;
in vec4 in_Color;

out vec4 Color;
 
uniform mat4 m;
uniform mat4 v;
uniform mat4 mvp;

void main(void) 
{
    gl_Position = mvp * vec4(in_Position, 1);

	Color = in_Color;
}