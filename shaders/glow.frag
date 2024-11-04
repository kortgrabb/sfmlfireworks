uniform sampler2D texture;
uniform float glowStrength;
uniform float time;

void main() {
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    float glow = sin(time * 10.0) * 0.2 + 0.8;
    vec4 glowColor = vec4(1.0, 1.0, 0.0, pixel.a * glowStrength * glow);
    gl_FragColor = pixel + glowColor;
}
