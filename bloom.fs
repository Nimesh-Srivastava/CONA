#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// Settings
const float threshold = 0.5;
const float intensity = 1.2;
const int samples = 5; // Increase for smoother blur
const float spread = 2.5;

void main()
{
    vec4 texel = texture(texture0, fragTexCoord);
    
    // Simple Gaussian-ish blur hack for glow
    vec4 glow = vec4(0.0);
    vec2 size = 1.0 / vec2(textureSize(texture0, 0));
    
    int count = 0;
    for (int x = -samples; x <= samples; x++)
    {
        for (int y = -samples; y <= samples; y++)
        {
            vec2 offset = vec2(float(x), float(y)) * spread * size;
            vec4 p = texture(texture0, fragTexCoord + offset);
            
            // Only bright pixels contribute to glow
            float brightness = dot(p.rgb, vec3(0.299, 0.587, 0.114));
            if (brightness > threshold) {
                glow += p;
                count++;
            }
        }
    }
    
    if (count > 0) {
        glow /= float(count);
    }
    
    finalColor = texel + glow * intensity;
}
