#version 410 core

in vec2 uv;
out vec4 FragColor;

uniform vec2 resolution;
uniform int maxIterations;
uniform vec3 color;
uniform vec3 colorBg;
uniform bool adaptiveIterations;

// Always use single precision in shader (macOS doesn't support double in GLSL)
// But we can use high precision qualifiers and better calculation techniques
uniform float zoom;
uniform vec2 offset;

#ifdef USE_DOUBLE_PRECISION
// When double precision is requested, we use high precision floats
// and implement better numerical techniques
#define PRECISION_QUALIFIER highp
#else
#define PRECISION_QUALIFIER mediump
#endif

vec3 getColor(float t, vec3 color, vec3 colorBg) {
    return mix(colorBg, color, t);
}

// Mandelbrot calculation function with precision qualifiers
vec4 mandelbrot(PRECISION_QUALIFIER vec2 c, int maxIter, vec3 color, vec3 colorBg) {
    PRECISION_QUALIFIER vec2 z = vec2(0.0);
    int iter = 0;
    
    for (int i = 0; i < maxIter; i++) {
        PRECISION_QUALIFIER float z_squared = dot(z, z);
        if (z_squared > 4.0) {
            float t = float(iter) / float(maxIterations);
            return vec4(getColor(t, color, colorBg), 1.0);
        }
        
        // z = z^2 + c with explicit precision
        PRECISION_QUALIFIER float zx2 = z.x * z.x;
        PRECISION_QUALIFIER float zy2 = z.y * z.y;
        PRECISION_QUALIFIER float zxy = z.x * z.y;
        
        z = vec2(zx2 - zy2, 2.0 * zxy) + c;
        iter = i;
    }
    
    return vec4(0.0, 0.0, 0.0, 1.0);
}

void main() {
    // Calculate relative coordinates from screen center
    // This approach maintains precision at high zoom levels
    PRECISION_QUALIFIER vec2 screenPos = gl_FragCoord.xy;
    PRECISION_QUALIFIER vec2 screenCenter = resolution * 0.5;
    
    // Calculate offset from center in screen pixels
    PRECISION_QUALIFIER vec2 pixelOffset = screenPos - screenCenter;
    
    // Convert to complex plane coordinates relative to zoom center
    PRECISION_QUALIFIER float aspectRatio = resolution.x / resolution.y;
    PRECISION_QUALIFIER vec2 c = offset + vec2(
        (pixelOffset.x / resolution.x) * zoom * aspectRatio * 2.0,
        -(pixelOffset.y / resolution.y) * zoom * 2.0
    );
    
    // Adaptive iteration count based on zoom level
    int adaptiveMaxIter = maxIterations;
    if (adaptiveIterations) {
        // Increase iterations as we zoom in (logarithmic scaling)
        PRECISION_QUALIFIER float zoomFactor = 1.0 / zoom;
        adaptiveMaxIter = int(float(maxIterations) * (1.0 + log2(max(zoomFactor, 1.0)) * 0.1));
        adaptiveMaxIter = min(adaptiveMaxIter, 2000); // Cap at reasonable maximum
    }
    
    // Calculate Mandelbrot iterations
    FragColor = mandelbrot(c, adaptiveMaxIter, color, colorBg);
}