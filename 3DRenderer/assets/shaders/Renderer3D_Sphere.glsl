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
    vec3 Albedo;
    float Roughness;
    float Metallic;
    float Opacity;
    float IOR;
};

layout(std140, binding = 1) uniform SphereData
{
    Sphere u_Spheres[10];
    int u_SphereCount;
};

// Light pos stays as a normal uniform
uniform vec3 u_LightPos;
// Bounce count for light reflections
uniform int u_BounceCount;
// Sampling count for soft shadows and glossy reflections
uniform int u_SampleCount;

// define a hit structure to store hit information
struct HitInfo {
    vec3 Position;
    vec3 Normal;
    int ObjectID; // -1 for none, 0+ for spheres, -2 for floor, -3 for light bulb
};

// function to trace a ray
HitInfo TraceRay(vec3 rayOrigin, vec3 rayDir)
{
    HitInfo hit;
    hit.ObjectID = -1; // Default to no hit
    float closestT = 999999.0;

    // Check spheres
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
            if (tFront > 0.001 && tFront < closestT)
            {
                closestT = tFront;
                hit.ObjectID = i; // Regular Sphere
                hit.Position = rayOrigin + rayDir * tFront;
                hit.Normal = normalize(hit.Position - u_Spheres[i].Position);
            }
        }
    }

    // Check floor
    float floorY = -2.0;
    if (rayDir.y < 0.0) // Only check if the ray is actually pointing downward!
    {
        float tFloor = (floorY - rayOrigin.y) / rayDir.y;
        if (tFloor > 0.001 && tFloor < closestT)
        {
            closestT = tFloor;
            hit.ObjectID = -2; // Special ID: Floor
            hit.Position = rayOrigin + rayDir * tFloor;
            hit.Normal = vec3(0.0, 1.0, 0.0); // Floor normal ALWAYS points straight up
        }
    }

    // Check light bulb
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
            hit.ObjectID = -3; // Special ID: Light Bulb
            hit.Position = rayOrigin + rayDir * tLight;
            hit.Normal = vec3(0.0); // Emissive objects don't need normals for shading
        }
    }
    return hit;
}

// function to calculate light transmission through transparent objects
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
                float alpha = u_Spheres[i].Opacity; // Use the Opacity property to determine how much light is blocked
                
                // 1. If the object is fully opaque, the shadow is completely black.
                if (alpha >= 0.99) {
                    return vec3(0.0);
                }
                
                // 2. Transparent objects tint the light and reduce its intensity
                // Mix between pure white (no tint) and the object's color based on alpha
                vec3 filterColor = mix(vec3(1.0), u_Spheres[i].Albedo, alpha);
                
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

// A fast, high-quality random number generator (PCG Hash)
uint pcg_hash(uint seed)
{
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// Generates a random float between 0.0 and 1.0
float randomFloat(inout uint seed)
{
    seed = pcg_hash(seed);
    return float(seed) / 4294967295.0; // Divide by max uint32
}

// Generates a random point inside a 3D unit sphere
vec3 randomDirection(inout uint seed)
{
    float x = randomFloat(seed) * 2.0 - 1.0;
    float y = randomFloat(seed) * 2.0 - 1.0;
    float z = randomFloat(seed) * 2.0 - 1.0;
    return normalize(vec3(x, y, z));
}

void main()
{
    // Create a unique seed for this specific pixel
    // (Combining X, Y coords and the current sample number)
    // gl_FragCoord is strictly positive, preventing the negative-to-uint clamping bug!
    uint pixelIndex = uint(gl_FragCoord.x) + uint(gl_FragCoord.y) * 1920u;
    uint rngState = pixelIndex;

    vec3 accumulatedColor = vec3(0.0);
    // --- The Multisampling Loop ---
    for (int sampleIdx = 0; sampleIdx < u_SampleCount; sampleIdx++)
    {
        // Advance the RNG state for this sample
        rngState = pcg_hash(rngState + uint(sampleIdx));

        // --- 1. Calculate Ray Origin and Direction ---
        // Un-project screen coordinates into world space directions
        vec4 target = u_InverseProjection * vec4(v_ScreenCoord.x, v_ScreenCoord.y, 1.0, 1.0);
        vec3 rayDir = vec3(u_InverseView * vec4(normalize(vec3(target) / target.w), 0.0));
        
        // Extract camera position from the translation part of the inverse view matrix
        vec3 rayOrigin = vec3(u_InverseView[3]); 

        // --- 2. Ray Bouncing Setup ---
        vec3 sampleColor = vec3(0.0);
        float throughput = 1.0; 

        for (int bounce = 0; bounce < u_BounceCount; bounce++) 
        {
            HitInfo hit = TraceRay(rayOrigin, rayDir);

            // 2. Evaluate the Hit
            if (hit.ObjectID >= 0) // We hit a Sphere!
            {
                vec3 lightDir = normalize(u_LightPos - hit.Position);
                float lightIntensity = max(dot(hit.Normal, lightDir), 0.0);
                vec3 transmission = CalculateLightTransmission(hit.Position, hit.Normal);
                vec3 finalIntensity = (vec3(lightIntensity) * transmission) + vec3(0.1); 

                float alpha = u_Spheres[hit.ObjectID].Opacity; 
                float metallic = u_Spheres[hit.ObjectID].Metallic; // Use Metallic for reflections!

                // 1. Diffuse Color: The color of the sphere itself. 
                // Metals don't have diffuse color, so we fade it out as Metallic approaches 1.0
                vec3 diffuseColor = u_Spheres[hit.ObjectID].Albedo * finalIntensity;
                sampleColor += diffuseColor * (1.0 - metallic) * alpha * throughput;

                // 2. Update Throughput for the reflection bounce.
                // A mirror continues with 100% energy. A matte surface drops the energy to 0%.
                throughput *= metallic; 

                // Optimization: If the surface isn't reflective, stop bouncing!
                if (throughput <= 0.01) break;
                if (u_Spheres[hit.ObjectID].Metallic <= 0.01) break;

                // --- 3. Prepare for Next Bounce ---
                float roughness = u_Spheres[hit.ObjectID].Roughness;

                // --- THE CONE MATH (UPGRADED) ---
                vec3 perfectReflection = reflect(rayDir, hit.Normal);
                vec3 randomVec = randomDirection(rngState);

                // 1. If the random vector points inside the sphere, flip it so it points outward!
                if (dot(randomVec, hit.Normal) < 0.0) {
                    randomVec = -randomVec;
                }

                // 2. Create a perfectly diffuse, random bounce
                vec3 diffuseBounce = normalize(hit.Normal + randomVec * 0.5);

                // 3. Mix between mirror and diffuse based on roughness.
                // (Squaring the roughness gives a much more natural, linear slider feel)
                rayDir = normalize(mix(perfectReflection, diffuseBounce, roughness * roughness));

                // Apply the offset and move to the next bounce
                rayOrigin = hit.Position + (hit.Normal * 0.001);
            }
            else if (hit.ObjectID == -2) // We hit the Floor
            {   
                // Generate the Checkerboard Pattern
                float tileSize = 2.0; 
                float pattern = mod(floor(hit.Position.x / tileSize) + floor(hit.Position.z / tileSize), 2.0);
                
                // Mix between a dark grey and light grey based on the pattern
                vec3 floorColor = mix(vec3(0.15), vec3(0.4), pattern);
                
                // Lighting & Shadows for the floor
                vec3 lightDir = normalize(u_LightPos - hit.Position);
                float lightIntensity = max(dot(hit.Normal, lightDir), 0.0);
                vec3 transmission = CalculateLightTransmission(hit.Position, hit.Normal);
                vec3 finalIntensity = (vec3(lightIntensity) * transmission) + vec3(0.1);

                sampleColor += floorColor * finalIntensity * throughput;
                break; // The floor is solid rock. Stop bouncing!
            }
            else if (hit.ObjectID == -3) // We hit the Light Bulb
            {
                vec3 bulbColor = vec3(1.0, 0.9, 0.7);
                sampleColor += bulbColor * throughput;
                break; 
            }
            else
            {
                // We hit the empty sky
                float gradientFactor = (v_ScreenCoord.y + 1.0) * 0.5;
                vec3 skyColor = mix(vec3(0.05, 0.05, 0.05), vec3(0.1, 0.1, 0.2), gradientFactor);
                
                // The sky fills whatever throughput is left over!
                sampleColor += skyColor * throughput;
                break; // Nothing left to hit
            }
        }
        accumulatedColor += sampleColor;
    }

    FragColor = vec4(accumulatedColor / float(u_SampleCount), 1.0);
}