//--------------------------
// Renderer3D Sphere Shader
// --------------------------

#type vertex
#version 450 core
layout(location = 0) in vec3 a_Position;

out vec2 v_ScreenCoord;

void main()
{
    v_ScreenCoord = a_Position.xy;
    gl_Position = vec4(a_Position, 1.0);
}

#type fragment
#version 450 core
layout(location = 0) out vec4 FragColor;

in vec2 v_ScreenCoord;

// --- Binding 0: Camera Data ---
layout(std140, binding = 0) uniform CameraData
{
    mat4 u_InverseProjection;
    mat4 u_InverseView;
};

// --- Binding 1: Sphere Data ---
struct Sphere {
    vec3 Position;
    float Radius;
    vec4 Color;
};

layout(std140, binding = 1) uniform SphereData
{
    Sphere u_Spheres[10];
    int u_SphereCount;
};

// Light pos stays as a normal uniform
uniform vec3 u_LightPos;

vec3 CalculateLightTransmission(vec3 hitPoint, vec3 normal)
{
    vec3 lightVec = u_LightPos - hitPoint;
    float distToLight = length(lightVec);
    vec3 rayDir = normalize(lightVec);

    vec3 rayOrigin = hitPoint + (normal * 0.001); 
    
    // 1.0 means 100% of the white light is getting through
    vec3 lightMultiplier = vec3(1.0); 

    for (int i = 0; i < u_SphereCount; i++)
    {
        vec3 oc = rayOrigin - u_Spheres[i].Position;
        float a = dot(rayDir, rayDir);
        float b = 2.0 * dot(oc, rayDir);
        float c = dot(oc, oc) - (u_Spheres[i].Radius * u_Spheres[i].Radius);

        float discriminant = (b * b) - (4.0 * a * c);

        if (discriminant > 0.0)
        {
            float t = (-b - sqrt(discriminant)) / (2.0 * a);
            
            if (t > 0.0 && t < distToLight)
            {
                float alpha = u_Spheres[i].Color.a;
                
                // 1. If the object is fully opaque, the shadow is completely black.
                if (alpha >= 0.99) {
                    return vec3(0.0);
                }
                
                // 2. Transparent objects tint the light and reduce its intensity
                // Mix between pure white (no tint) and the object's color based on alpha
                vec3 filterColor = mix(vec3(1.0), u_Spheres[i].Color.rgb, alpha);
                
                // Multiply the remaining light by the filter, and reduce by (1 - alpha)
                lightMultiplier *= filterColor * (1.0 - alpha);
                
                // 3. Optimization: If the light is basically blocked, stop checking
                if (length(lightMultiplier) <= 0.01) {
                    return vec3(0.0);
                }
            }
        }
    }

    return lightMultiplier;
}

void main()
{
    // --- 1. Calculate Ray Origin and Direction ---
    // Un-project screen coordinates into world space directions
    vec4 target = u_InverseProjection * vec4(v_ScreenCoord.x, v_ScreenCoord.y, 1.0, 1.0);
    vec3 rayDir = vec3(u_InverseView * vec4(normalize(vec3(target) / target.w), 0.0));
    
    // Extract camera position from the translation part of the inverse view matrix
    vec3 rayOrigin = vec3(u_InverseView[3]); 

    // --- 2. Ray-Sphere Intersection Loop ---
    // Closest t values
    float Ts[10];

    // Loop through the batched data
    for (int i = 0; i < u_SphereCount; i++)
    {
        Ts[i] = -1.0; // Default to no hit
        vec3 oc = rayOrigin - u_Spheres[i].Position;
        float a = dot(rayDir, rayDir);
        float b = 2.0 * dot(oc, rayDir);
        float c = dot(oc, oc) - (u_Spheres[i].Radius * u_Spheres[i].Radius);

        float discriminant = (b * b) - (4.0 * a * c);

        if (discriminant > 0.0)
        {
            // Calculate the nearest hit point (the minus in the quadratic formula)
            float t = (-b - sqrt(discriminant)) / (2.0 * a);
            
            Ts[i] = t;
        }
    }

    // Find the closest solid color hit
    float closestSolidSphereT = 999999.0;
    int hitSphereIndex = -1;
    vec3 pixelColor = vec3(0.0); // Default background color

    for (int i = 0; i < u_SphereCount; i++)
    {
        if (Ts[i] > 0.0 && Ts[i] < closestSolidSphereT && u_Spheres[i].Color.a >= 0.99)
        {
            closestSolidSphereT = Ts[i];
            hitSphereIndex = i;
            pixelColor = u_Spheres[i].Color.rgb;
        }
    }

    float cumulativeAlpha = 1.0;
    // check for the closest transparent hit for shading purposes
    for (int i = 0; i < u_SphereCount; i++)
    {
        if (Ts[i] > 0.0 && Ts[i] < closestSolidSphereT)
        {
            // pixelColor = mix(pixelColor, u_Spheres[i].Color.rgb, u_Spheres[i].Color.a);

            vec3 hitPoint = rayOrigin + (rayDir * Ts[i]);
            vec3 normal = normalize(hitPoint - u_Spheres[i].Position);
            vec3 lightDir = normalize(u_LightPos - hitPoint);
            
            // Base direct lighting
            float lightIntensity = max(dot(normal, lightDir), 0.0);
            
            // Get our colored shadow multiplier
            vec3 transmission = CalculateLightTransmission(hitPoint, normal);

            // Apply transmission ONLY to the direct light. Add 0.1 ambient so shadows aren't pitch black.
            vec3 finalIntensity = (vec3(lightIntensity) * transmission) + vec3(0.1);

            pixelColor = mix(pixelColor, u_Spheres[i].Color.rgb * finalIntensity, u_Spheres[i].Color.a);
            cumulativeAlpha *= (1.0 - u_Spheres[i].Color.a);
        }
    }

    // final blend for the closest solid hit (if there is one)
    if (hitSphereIndex != -1)
    {
        vec3 hitPoint = rayOrigin + (rayDir * closestSolidSphereT);
        vec3 normal = normalize(hitPoint - u_Spheres[hitSphereIndex].Position);
        vec3 lightDir = normalize(u_LightPos - hitPoint);
        
        // Base direct lighting
        float lightIntensity = max(dot(normal, lightDir), 0.0);
        
        // Get our colored shadow multiplier
        vec3 transmission = CalculateLightTransmission(hitPoint, normal);

        // Apply transmission ONLY to the direct light. Add 0.1 ambient so shadows aren't pitch black.
        vec3 finalIntensity = (vec3(lightIntensity) * transmission) + vec3(0.1);

        pixelColor = pixelColor = mix(pixelColor, u_Spheres[hitSphereIndex].Color.rgb * finalIntensity, cumulativeAlpha);
    }
    
    FragColor = vec4(pixelColor, 1.0);

    // // --- 3. Shading ---
    // if (hitSphereIndex != -1)
    // {
    //     vec3 hitPoint = rayOrigin + (rayDir * closestSolidSphereT);
    //     vec3 normal = normalize(hitPoint - u_Spheres[hitSphereIndex].Position);
    //     vec3 lightDir = normalize(u_LightPos - hitPoint);
        
    //     // Base direct lighting
    //     float lightIntensity = max(dot(normal, lightDir), 0.0);
        
    //     // Get our colored shadow multiplier
    //     vec3 transmission = CalculateLightTransmission(hitPoint, normal);

    //     // Apply transmission ONLY to the direct light. Add 0.1 ambient so shadows aren't pitch black.
    //     vec3 finalIntensity = (vec3(lightIntensity) * transmission) + vec3(0.1);

    //     vec3 finalColor = pixelColor * finalIntensity;

    //     // Keep the alpha of the hit object so the background can show through (if we add background blending later)
    //     FragColor = vec4(finalColor, 1.0);
    // }
    // else
    // {
    //     // No hit, just output the background color (black in this case)
    //     FragColor = vec4(pixelColor, 1.0);
    // }
}