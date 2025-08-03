# 🎨 Vulkan Shader Loader

A powerful, easy-to-use Vulkan application for loading and testing custom shaders in real-time. Perfect for shader developers, graphics programmers, and anyone wanting to experiment with beautiful visual effects.

![Shader Demo](https://img.shields.io/badge/Graphics-Vulkan-red) ![Platform](https://img.shields.io/badge/Platform-Linux-blue) ![License](https://img.shields.io/badge/License-MIT-green)

## 🚀 Features

- **🔥 Real-time Shader Loading** - Load and test vertex, fragment, and compute shaders instantly
- **🎯 Easy Template System** - Pre-made shader templates with clear instructions
- **⚡ Hot Reload** - Edit shaders and see changes immediately
- **🖥️ Cross-Platform** - Built with modern Vulkan API
- **🎨 Beautiful Samples** - Includes stunning example shaders to get you started
- **📁 Simple File Structure** - Just edit files and run!

## 📦 What's Included

```
ShaderLoader/
├── app                          # Main executable
├── shaders/                     # Shader templates directory
│   ├── custom_vertex.vert       # ✏️ Edit this for vertex shaders
│   ├── custom_fragment.frag     # ✏️ Edit this for fragment shaders
│   ├── custom_compute.comp      # ✏️ Edit this for compute shaders
│   ├── *.spv                    # Compiled SPIR-V files (auto-generated)
└── README.md                    # This file
```

## 🛠️ Requirements

- **Linux** (Ubuntu/Debian/Fedora/Arch)
- **Vulkan drivers** installed for your GPU
- **GLFW** and **Vulkan SDK** (for compiling custom shaders)

### Quick Install Dependencies (Ubuntu/Debian):
```bash
sudo apt update
sudo apt install vulkan-tools vulkan-validationlayers-dev libglfw3-dev glslc
```

### Quick Install Dependencies (Arch):
```bash
sudo pacman -S vulkan-tools vulkan-validation-layers glfw-wayland shaderc
```

## 🎮 How to Use

### 1. **Run the App**
```bash
./run_shader_loader.sh
```
**Note:** Use the launcher script instead of running `./app` directly - this ensures shader files are found correctly.

You should see a colorful triangle with smooth color gradients!

### 2. **Edit Custom Shaders**
Open the shader files in your favorite text editor:

**For Vertex Shaders:**
```bash
nano shaders/custom_vertex.vert
# Or use any editor: code, vim, gedit, etc.
```

**For Fragment Shaders:**
```bash
nano shaders/custom_fragment.frag
```

### 3. **Compile Your Changes**
```bash
cd shaders
glslc custom_vertex.vert -o custom_vertex.vert.spv
glslc custom_fragment.frag -o custom_fragment.frag.spv
```

### 4. **See Your Results**
```bash
./app
```
Your custom shaders will be loaded automatically!

## 🎨 Example Workflow

Let's create a pulsing red triangle:

**1. Edit the vertex shader:**
```glsl
// In custom_vertex.vert
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),  // All red
    vec3(1.0, 0.0, 0.0),  // All red  
    vec3(1.0, 0.0, 0.0)   // All red
);
```

**2. Edit the fragment shader:**
```glsl
// In custom_fragment.frag
void main() {
    float pulse = sin(gl_FragCoord.x * 0.01) * 0.5 + 0.5;
    outColor = vec4(fragColor * pulse, 1.0);
}
```

**3. Compile and run:**
```bash
cd shaders
glslc custom_vertex.vert -o custom_vertex.vert.spv
glslc custom_fragment.frag -o custom_fragment.frag.spv
cd ..
./app
```

You'll see a pulsing red triangle! 🔴

## 📚 Shader Templates Explained

### Vertex Shader Template (`custom_vertex.vert`)
```glsl
#version 450

// Define vertex positions
vec2 positions[3] = vec2[](
    vec2(0.0, 0.5),   // top
    vec2(-0.5, -0.5), // bottom left
    vec2(0.5, -0.5)   // bottom right
);

// Define colors for each vertex
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
```

**Key Points:**
- Change `positions[]` to modify shape
- Change `colors[]` to modify vertex colors
- Add animations using `gl_VertexIndex` as time
- Output `fragColor` to pass data to fragment shader

### Fragment Shader Template (`custom_fragment.frag`)
```glsl
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

**Key Points:**
- `fragColor` comes from vertex shader
- `outColor` is the final pixel color
- Use `gl_FragCoord` for screen position effects
- Add noise, patterns, or animations here

## 🎯 Advanced Examples

### Animated Rotating Triangle
```glsl
// In vertex shader main()
float time = float(gl_VertexIndex) * 0.1;
float angle = time;
mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
vec2 pos = rotation * positions[gl_VertexIndex];
gl_Position = vec4(pos, 0.0, 1.0);
```

### Psychedelic Fragment Effect
```glsl
// In fragment shader main()
vec2 uv = gl_FragCoord.xy * 0.01;
float wave = sin(uv.x * 10.0) * sin(uv.y * 10.0);
vec3 color = fragColor + wave * 0.5;
outColor = vec4(color, 1.0);
```

### Pulsing Colors
```glsl
// In fragment shader main()
float pulse = sin(gl_FragCoord.x * 0.05 + gl_FragCoord.y * 0.03) * 0.5 + 0.5;
outColor = vec4(fragColor * pulse, 1.0);
```

## 🔧 Troubleshooting

### Black Screen?
1. **Check compilation:** Make sure your shaders compile without errors
   ```bash
   glslc custom_vertex.vert -o custom_vertex.vert.spv
   ```
2. **Check file paths:** Ensure `.spv` files are in the `shaders/` directory
3. **Try simple shaders:** Use the default templates first

### Compilation Errors?
1. **Check GLSL syntax:** Make sure your shader code is valid GLSL 4.5
2. **Match interfaces:** Vertex shader outputs must match fragment shader inputs
3. **Use layout locations:** Always specify `layout(location = N)`

### App Won't Start?
1. **Check Vulkan:** Run `vulkaninfo` to verify Vulkan is installed
2. **Update drivers:** Make sure your GPU drivers support Vulkan
3. **Install dependencies:** See requirements section above

## 🎨 Shader Resources

### Learning Resources
- [Learn OpenGL - Shaders](https://learnopengl.com/Getting-started/Shaders)
- [Shadertoy](https://shadertoy.com) - Online shader editor
- [The Book of Shaders](https://thebookofshaders.com/)
- [Vulkan Tutorial](https://vulkan-tutorial.com/)

### Useful GLSL Functions
```glsl
// Math
sin(), cos(), tan()          // Trigonometry
length(), distance()         // Vector operations
mix(a, b, t)                // Linear interpolation
smoothstep(edge0, edge1, x) // Smooth transitions
fract(), floor(), ceil()    // Number manipulation

// Graphics
gl_FragCoord                // Screen position
gl_VertexIndex             // Current vertex number
texture()                  // Sample textures
normalize()                // Normalize vectors
```

## 🏗️ Building from Source

If you want to modify the application itself:

```bash
# Clone and build
git clone <your-repo>
cd ShaderLoader
mkdir build && cd build
cmake ..
make

# Copy executable and shaders
cp app ../release/ShaderLoader/
cp -r ../shaders ../release/ShaderLoader/
```

## 📄 License

MIT License - Feel free to use this for learning, projects, or commercial work!

## 🤝 Contributing

Found a bug? Want to add features? Pull requests welcome!

## 💫 Gallery

Try these shader combinations for stunning effects:

- **🌈 Rainbow Triangle:** Cycle colors over time
- **⚡ Electric Effect:** Add noise and lightning patterns  
- **🌊 Wave Distortion:** Animate vertex positions with sine waves
- **🎆 Particle System:** Use fragment shader for particle effects
- **🔥 Fire Simulation:** Combine noise with color gradients

---

**Happy Shader Coding!** 🎨✨

*Made with ❤️ for the graphics programming community*
