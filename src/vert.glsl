#version 450 core

layout(location = 0) in vec4 p;
layout(location = 1) in vec4 n;
layout(location = 2) in vec4 t;

out mat3 gTBN;
out vec3 gP;
out vec2 gUV;
flat out int gMID;

uniform mat4 MVP;
uniform mat4 M;
uniform mat3 IM;

void main() {
	gl_Position = MVP * vec4(p.xyz, 1.0);
    gP = vec3(M * vec4(p.xyz, 1.0));
    gUV = vec2(p.w, n.w);
    gMID = int(t.w);
    vec3 T = normalize(vec3(M * vec4(IM * t.xyz, 0.0)));
    vec3 N = normalize(vec3(M * vec4(IM * n.xyz, 0.0)));
    vec3 B = cross(T, N);
    gTBN = mat3(T, B, N);
}
