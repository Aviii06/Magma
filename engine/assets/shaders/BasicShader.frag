#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 v_worldPos;

layout(location = 0) out vec4 outColor;

vec4 directionalLight(vec3 normal, vec3 lightDir, vec3 objectColor, vec3 lightColor)
{
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 ambient = lightColor;
    vec3 diffuse = max(dot(normal, lightDir), 0.0f) * lightColor;

    vec4 color = vec4((ambient + diffuse) * objectColor, 1.0f);
    return color;
}

void main()
{
    vec3 ax = dFdx(v_worldPos);
    vec3 ay = dFdy(v_worldPos);
    vec3 normal = normalize(cross(ax, ay));

    //    outColor = vec4(fragColor, 1.0);

    outColor = directionalLight(normal, vec3(1.0, 1.0, 1.0), vec3(1.0, 0.1, 0.4), vec3(1.0, 1.0, 1.0));
}