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

uniform sampler2D u_Texture;
uniform bool u_Horizontal; // C++ toggles this back and forth during the Ping-Pong loop

// Standard Gaussian weights for a beautiful, soft blur
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    // Automatically calculate the exact size of one pixel based on the current viewport
    vec2 tex_offset = 1.0 / textureSize(u_Texture, 0); 
    
    // Start with the current pixel's color, weighted the heaviest
    vec3 result = texture(u_Texture, v_TexCoord).rgb * weight[0]; 

    // Sample the neighboring pixels
    if (u_Horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(u_Texture, v_TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(u_Texture, v_TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(u_Texture, v_TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(u_Texture, v_TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    
    FragColor = vec4(result, 1.0);
}