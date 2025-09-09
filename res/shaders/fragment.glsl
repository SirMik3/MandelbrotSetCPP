#version 330 core

in vec2 uv;
out vec4 FragColor;

uniform vec2 resolution;
uniform float zoom;
uniform vec2 offset;
uniform int maxIterations;
uniform vec3 color;
uniform vec3 colorBg;
uniform bool adaptiveIterations;

vec3 getColor(float t, vec3 color, vec3 colorBg) {
    return mix(colorBg, color, t);
}

// Mandelbrot calculation function
vec4 mandelbrot(vec2 c, int maxIter, vec3 color, vec3 colorBg) {
    vec2 z = vec2(0.0);
    int iter = 0;
    
    for (int i = 0; i < maxIter; i++) {
        if (dot(z, z) > 4.0) {
            float t = float(iter) / float(maxIterations);
            return vec4(getColor(t, color, colorBg), 1.0);
        }
        
        // z = z^2 + c
        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
        iter = i;
    }
    
    return vec4(0.0, 0.0, 0.0, 1.0);
}

void main() {
    // Calculate relative coordinates from screen center
    // This approach maintains precision at high zoom levels
    vec2 screenPos = gl_FragCoord.xy;
    vec2 screenCenter = resolution * 0.5;
    
    // Calculate offset from center in screen pixels
    vec2 pixelOffset = screenPos - screenCenter;
    
    // Convert to complex plane coordinates relative to zoom center
    float aspectRatio = resolution.x / resolution.y;
    vec2 c = offset + vec2(
        (pixelOffset.x / resolution.x) * zoom * aspectRatio * 2.0,
        -(pixelOffset.y / resolution.y) * zoom * 2.0
    );
    
    // Adaptive iteration count based on zoom level
    int adaptiveMaxIter = maxIterations;
    if (adaptiveIterations) {
        // Increase iterations as we zoom in (logarithmic scaling)
        float zoomFactor = 1.0 / zoom;
        adaptiveMaxIter = int(float(maxIterations) * (1.0 + log2(max(zoomFactor, 1.0)) * 0.1));
        adaptiveMaxIter = min(adaptiveMaxIter, 2000); // Cap at reasonable maximum
    }
    
    // Calculate Mandelbrot iterations
    FragColor = mandelbrot(c, adaptiveMaxIter, color, colorBg);
}