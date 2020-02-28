#version 460

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 0) out vec4 outColor;

layout (std140, binding = 0) uniform Lighting
{
    vec4 lightPos; // if w=0 this is lightDir
    vec3 ambient;
    vec3 diffuse;
    vec4 specular; // w is shininess
} u;

void main()
{
    // Renormalize the normal after interpolation
    vec3 normal = normalize(inNormal);

    // Calculate light direction (i.e. vector that points *towards* the light source)
    vec3 lightDir;
    if (u.lightPos.w != 0.0)
        lightDir = normalize(u.lightPos.xyz - inWorldPos);
    else
        lightDir = -u.lightPos.xyz;
    vec3 viewDir = normalize(-inWorldPos);

    // Calculate diffuse factor
    float diffuse = max(0.0, dot(normal,lightDir));

    // Calculate specular factor (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specular = pow(max(0.0, dot(normal,halfwayDir)), u.specular.w);

    // Calculate the color
    vec3 color =
        u.ambient +
        u.diffuse*vec3(diffuse) +
        u.specular.xyz*vec3(specular);

    // Reinhard tone mapping
    vec3 mappedColor = color / (vec3(1.0) + color);

    // Output this color (no need to gamma adjust since the framebuffer is sRGB)
    outColor = vec4(mappedColor, 1.0);
}
