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

    // --- 2. Ray Bouncing Setup ---
    vec3 finalColor = vec3(0.0);
    float throughput = 1.0; 

    for (int bounce = 0; bounce < 3; bounce++) 
    {
        float closestT = 999999.0;
        float currentExitT = 0.0;
        int hitObjectID = -1; // Changed name for clarity!

        // --- A. Check the Spheres ---
        for (int i = 0; i < u_SphereCount; i++)
        {
            vec3 oc = rayOrigin - u_Spheres[i].Position;
            float a = dot(rayDir, rayDir);
            float b = 2.0 * dot(oc, rayDir);
            float c = dot(oc, oc) - (u_Spheres[i].Radius * u_Spheres[i].Radius);

            float discriminant = (b * b) - (4.0 * a * c);

            if (discriminant > 0.0)
            {
                float tFront = (-b - sqrt(discriminant)) / (2.0 * a);
                float tBack = (-b + sqrt(discriminant)) / (2.0 * a);
                
                if (tFront > 0.001 && tFront < closestT)
                {
                    closestT = tFront;
                    hitObjectID = i; // Regular Sphere
                    currentExitT = tBack; 
                }
            }
        }

        // --- B. Check the Floor (Y = -2.0) ---
        float floorY = -2.0;
        if (rayDir.y < 0.0) // Only check if the ray is actually pointing downward!
        {
            float tFloor = (floorY - rayOrigin.y) / rayDir.y;
            if (tFloor > 0.001 && tFloor < closestT)
            {
                closestT = tFloor;
                hitObjectID = -2; // Special ID: Floor
            }
        }

        // --- C. Check the Light Bulb ---
        float lightRadius = 0.2; // Make it a small orb
        vec3 ocLight = rayOrigin - u_LightPos;
        float bLight = 2.0 * dot(ocLight, rayDir);
        float cLight = dot(ocLight, ocLight) - (lightRadius * lightRadius);
        float discLight = (bLight * bLight) - (4.0 * cLight);
        
        if (discLight > 0.0)
        {
            float tLight = (-bLight - sqrt(discLight)) / 2.0; // Only care about the front
            if (tLight > 0.001 && tLight < closestT)
            {
                closestT = tLight;
                hitObjectID = -3; // Special ID: Light Bulb
            }
        }

        // 2. Evaluate the Hit
        if (hitObjectID >= 0) // We hit a Sphere!
        {
            vec3 hitPoint = rayOrigin + (rayDir * closestT);
            vec3 normal = normalize(hitPoint - u_Spheres[hitObjectID].Position);
            vec3 lightDir = normalize(u_LightPos - hitPoint);
            
            // Direct lighting & Shadows
            float lightIntensity = max(dot(normal, lightDir), 0.0);
            vec3 transmission = CalculateLightTransmission(hitPoint, normal);
            vec3 finalIntensity = (vec3(lightIntensity) * transmission) + vec3(0.1); // Keep 0.1 ambient

            float alpha = u_Spheres[hitObjectID].Color.a;

            // Blend this sphere's lit color into the final image, modulated by the current throughput
            finalColor += u_Spheres[hitObjectID].Color.rgb * finalIntensity * alpha * throughput;

            // Reduce the throughput for the next bounce (if alpha is 0.4, 60% of light continues)
            throughput *= (1.0 - alpha);

            // Optimization: If the throughput drops to basically zero, stop calculating!
            if (throughput <= 0.01) break;

            // 3. Push the ray completely through the sphere for the next loop!
            // We use the tBack value we saved earlier so we don't get trapped inside the volume
            rayOrigin = rayOrigin + (rayDir * (currentExitT + 0.001));
        }
        else if (hitObjectID == -2) // We hit the Floor
        {
            vec3 hitPoint = rayOrigin + (rayDir * closestT);
            vec3 normal = vec3(0.0, 1.0, 0.0); // Floor normal ALWAYS points straight up
            
            // Generate the Checkerboard Pattern
            float tileSize = 2.0; 
            float pattern = mod(floor(hitPoint.x / tileSize) + floor(hitPoint.z / tileSize), 2.0);
            
            // Mix between a dark grey and light grey based on the pattern
            vec3 floorColor = mix(vec3(0.15), vec3(0.4), pattern);
            
            // Lighting & Shadows for the floor
            vec3 lightDir = normalize(u_LightPos - hitPoint);
            float lightIntensity = max(dot(normal, lightDir), 0.0);
            vec3 transmission = CalculateLightTransmission(hitPoint, normal);
            vec3 finalIntensity = (vec3(lightIntensity) * transmission) + vec3(0.1);

            finalColor += floorColor * finalIntensity * throughput;
            break; // The floor is solid rock. Stop bouncing!
        }
        else if (hitObjectID == -3) // We hit the Light Bulb
        {
            // Emissive materials don't have shadows or shading. They just glow!
            vec3 bulbColor = vec3(1.0, 0.9, 0.7); // Warm slightly yellow white
            finalColor += bulbColor * throughput;
            break; // The bulb is completely opaque. Stop bouncing!
        }
        else
        {
            // We hit the empty sky
            float gradientFactor = (v_ScreenCoord.y + 1.0) * 0.5;
            vec3 skyColor = mix(vec3(0.05, 0.05, 0.05), vec3(0.1, 0.1, 0.2), gradientFactor);
            
            // The sky fills whatever throughput is left over!
            finalColor += skyColor * throughput;
            break; // Nothing left to hit
        }
    }

    FragColor = vec4(finalColor, 1.0);
}