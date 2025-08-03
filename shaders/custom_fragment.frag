#version 450

// ============================================================
// CUSTOM FRAGMENT SHADER - PASTE YOUR CODE BELOW THIS LINE
// ============================================================
// Instructions:
// 1. Replace the code below with your custom fragment shader
// 2. Input: fragColor from vertex shader
// 3. Output: outColor (final pixel color)
// 4. Save this file and run the app - it will auto-reload!
// ============================================================

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    // Simply output the interpolated vertex color
    outColor = vec4(fragColor, 1.0);
}

// ============================================================
// END OF CUSTOM SHADER CODE
// ============================================================
