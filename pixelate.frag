uniform sampler2D texture;
uniform float pixel_size;
uniform vec2 resolution;

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;
    vec2 pixelated = floor(uv * resolution / pixel_size) * pixel_size / resolution;
    gl_FragColor = texture2D(texture, pixelated);
}
