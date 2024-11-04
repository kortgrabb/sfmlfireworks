uniform float time;
uniform vec2 resolution;

void main() {
    vec2 coord = gl_FragCoord.xy / resolution;
    
    // Create animated gradient
    float pattern = sin(coord.x * 10.0 + time) * sin(coord.y * 10.0 + time) * 0.5 + 0.5;
    vec3 color = mix(vec3(0.1, 0.1, 0.2), vec3(0.2, 0.2, 0.3), pattern);
    
    gl_FragColor = vec4(color, 1.0);
}
