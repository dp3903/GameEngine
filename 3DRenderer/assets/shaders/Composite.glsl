#type vertex
#version 450 core
layout (location = 0) in vec3 a_Position;

out vec2 v_TexCoord;

void main()
{
    // Mathematically generate UV coordinates from the screen quad geometry
    v_TexCoord = (a_Position.xy * 0.5) + 0.5;
    
    gl_Position = vec4(a_Position, 1.0); 
}

#type fragment
#version 450 core
out vec4 FragColor;

in vec2 v_TexCoord;

// Make sure to bind these to Texture Unit 0 and Texture Unit 1 in your C++ code!
uniform sampler2D u_SceneTexture; 
uniform sampler2D u_BlurTexture;  

uniform float u_Exposure = 1.0; // Optional: Gives you control over scene brightness

void main()
{
    vec3 sceneColor = texture(u_SceneTexture, v_TexCoord).rgb;      
    vec3 bloomColor = texture(u_BlurTexture, v_TexCoord).rgb;
    
    // 1. Additive Blending (Optical Bloom)
    vec3 hdrColor = sceneColor + bloomColor;
    
    // 2. Exposure Tone Mapping (Slightly better than raw Reinhard)
    // This allows you to smoothly adjust the overall brightness of the scene
    vec3 mappedColor = vec3(1.0) - exp(-hdrColor * u_Exposure);
    
    // 3. Gamma Correction (CRITICAL)
    // Converts the linear light math from your raytracer into the sRGB color space your monitor expects
    const float gamma = 2.2;
    mappedColor = pow(mappedColor, vec3(1.0 / gamma));
    
    FragColor = vec4(mappedColor, 1.0);
}