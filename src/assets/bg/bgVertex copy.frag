#version 330 core

layout (location = 0) in vec3 aPos;

out vec2 worldPos;

void main()
{
    worldPos = aPos.xy;   // keep coordinates
    gl_Position = vec4(aPos, 1.0);
}


// const char* worldVertexShaderSource = "#version 330 core\n"
//     "layout (location = 0) in vec3 aPos;\n"
//     "\n"
//     "out vec2 worldPos;\n"
//     "\n"
//     "void main()\n"
//     "{\n"
//     "    worldPos = aPos.xy;   // keep coordinates\n"
//     "    gl_Position = vec4(aPos, 1.0);\n"
//     "}\0";

