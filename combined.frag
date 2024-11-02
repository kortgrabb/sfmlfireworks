uniform sampler2D texture;
uniform float pixel_size;
uniform vec2 resolution;
uniform float time;
uniform float crt_curve = 0.3;
uniform float scanline_intensity = 0.3;
uniform float chromatic_intensity = 0.5;

vec2 curve(vec2 uv) {
    uv = (uv - 0.5) * 2.0;
    uv *= 1.0 + crt_curve * dot(uv, uv);
    uv = (uv * 0.5) + 0.5;
    return uv;
}

vec3 sample_with_chromatic(vec2 uv) {
    float aberration = chromatic_intensity * 0.01;
    vec2 pixelated = floor(uv * resolution / pixel_size) * pixel_size / resolution;
    vec3 col;
    col.r = texture2D(texture, pixelated + vec2(aberration, 0.0)).r;
    col.g = texture2D(texture, pixelated).g;
    col.b = texture2D(texture, pixelated - vec2(aberration, 0.0)).b;
    return col;
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;
    
    // Apply CRT curvature
    uv = curve(uv);
    
    // Check if outside curved screen
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // Sample with chromatic aberration
    vec3 col = sample_with_chromatic(uv);
    
    // Add scanlines
    float scanline = sin(uv.y * resolution.y * 0.5) * 0.5 + 0.5;
    col *= 1.0 - scanline * scanline_intensity;
    
    // Add subtle vignette
    float vignette = uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);
    vignette = pow(vignette * 15.0, 0.25);
    col *= vignette;
    
    gl_FragColor = vec4(col, 1.0);
}
