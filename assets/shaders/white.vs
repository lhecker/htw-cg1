#version 330 core

in vec4 v_coord;

void main() {
	gl_Position = v_coord;
}