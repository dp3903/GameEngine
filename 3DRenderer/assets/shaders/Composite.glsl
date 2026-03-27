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

void main()
{
    vec3 sceneColor = texture(u_SceneTexture, v_TexCoord).rgb;      
    vec3 bloomColor = texture(u_BlurTexture, v_TexCoord).rgb;
    
    // 1. Additive Blending (Optical Bloom)
    vec3 hdrColor = sceneColor + bloomColor;
    FragColor = vec4(hdrColor, 1.0);
}