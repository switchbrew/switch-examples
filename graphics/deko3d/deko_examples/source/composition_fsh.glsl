#version 460

layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform sampler2D texAlbedo;
layout (binding = 1) uniform sampler2D texNormal;
layout (binding = 2) uniform sampler2D texViewDir;

layout (std140, binding = 0) uniform Lighting
{
    vec4 lightPos; // if w=0 this is lightDir
    vec3 ambient;
    vec3 diffuse;
    vec4 specular; // w is shininess
} u;

void main()
{
    // Uncomment the coordinate reversion below to observe the effects of tiled corruption
    ivec2 coord = /*textureSize(texAlbedo, 0) - ivec2(1,1) -*/ ivec2(gl_FragCoord.xy);

    // Retrieve values from the g-buffer
    vec4 albedo = texelFetch(texAlbedo, coord, 0);
    vec3 normal = texelFetch(texNormal, coord, 0).xyz;
    vec3 viewDir = texelFetch(texViewDir, coord, 0).xyz;

    // Calculate light direction (i.e. vector that points *towards* the light source)
    vec3 lightDir;
    if (u.lightPos.w != 0.0)
        lightDir = normalize(u.lightPos.xyz + viewDir);
    else
        lightDir = -u.lightPos.xyz;
    viewDir = normalize(viewDir);

    // Calculate diffuse factor
    float diffuse = max(0.0, dot(normal,lightDir));

    // Calculate specular factor (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specular = pow(max(0.0, dot(normal,halfwayDir)), u.specular.w);

    // Calculate the color
    vec3 color =
        u.ambient +
        albedo.rgb*u.diffuse*vec3(diffuse) +
        u.specular.xyz*vec3(specular);

    // Reinhard tone mapping
    vec3 mappedColor = albedo.a * color / (vec3(1.0) + color);

    // Output this color (no need to gamma adjust since the framebuffer is sRGB)
    outColor = vec4(mappedColor, albedo.a);
}
