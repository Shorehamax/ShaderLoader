#version 450

// ============================================================
// CUSTOM VERTEX SHADER - PASTE YOUR CODE BELOW THIS LINE
// ============================================================
// Instructions:
// 1. Replace the code below with your custom vertex shader
// 2. Make sure to keep the same output interface (fragColor)
// 3. Save this file and run the app - it will auto-reload!
// ============================================================

// Simple triangle vertices
vec2 positions[3] = vec2[](
    vec2(0.0, 0.5),   // top
    vec2(-0.5, -0.5), // bottom left
    vec2(0.5, -0.5)   // bottom right
);

// Simple bright colors
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),  // red
    vec3(0.0, 1.0, 0.0),  // green
    vec3(0.0, 0.0, 1.0)   // blue
);

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}

// ============================================================
// END OF CUSTOM SHADER CODE
// ============================================================
