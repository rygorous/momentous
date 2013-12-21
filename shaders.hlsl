#define TEX_WIDTH_LOG2 13
#define TEX_WIDTH (1 << TEX_WIDTH_LOG2)

struct CubeVert {
    float4 clip_pos : SV_Position;
    float3 world_pos : WorldPos; // .xyz = world space position
};

cbuffer CubeConsts : register(b0) {
    float4x4 clip_from_world;
    float3 world_down_vector;
    float cube_size;

    // diffuse trilight plus ambient
    float3 light_color_ambient;
    float3 light_color_key;
    float3 light_color_fill;
    float3 light_color_back;
    float3 light_dir;
};

CubeVert RenderCubeVertexShader(
    uint vertex_id : SV_VertexID,
    Texture2D tex_pos : register(t0),
    Texture2D tex_vel : register(t1)
)
{
    CubeVert v;
    uint cube_id = vertex_id >> 3;

    // fetch cube position and velocity from textures
    int3 fetch_coord = int3(cube_id & (TEX_WIDTH - 1), cube_id >> TEX_WIDTH_LOG2, 0);
    //float4 cube_pos = tex_pos.Load(fetch_coord);
    //float4 cube_vel = tex_vel.Load(fetch_coord);
    float4 cube_pos = 0;
    float4 cube_vel = float4(1, 0, 0, 0);

    // determine local coordinate system
    float3 x_axis = cube_vel.xyz;
    float3 z_axis = normalize(cross(x_axis, world_down_vector));
    float3 y_axis = normalize(cross(z_axis, x_axis));

    // generate cube vertex
    float3 world_pos = cube_pos.xyz;
    world_pos += (((vertex_id & 1) != 0) ? 1.0 : -1.0) * x_axis;
    world_pos += (((vertex_id & 2) != 0) ? cube_size : -cube_size) * y_axis;
    world_pos += (((vertex_id & 4) != 0) ? cube_size : -cube_size) * z_axis;

    // generate output vertex
    v.clip_pos = mul(clip_from_world, float4(world_pos, 1.0));
    v.world_pos = world_pos;
    return v;
}

float4 RenderCubePixelShader(
    CubeVert v
) : SV_Target
{
    // determine triangle plane from derivatives
    float3 dPos_dx = ddx(v.world_pos.xyz);
    float3 dPos_dy = ddy(v.world_pos.xyz);

    // world-space normal from tangents
    float3 world_normal = cross(dPos_dy, dPos_dx);

    // lighting model (trilight)
    float NdotL = dot(world_normal, light_dir) * rsqrt(dot(world_normal, world_normal));

    float3 diffuse_lit = light_color_ambient
        + saturate(NdotL) * light_color_key
        + (1.0 - abs(NdotL)) * light_color_fill
        + saturate(-NdotL) * light_color_back;

    return float4(diffuse_lit, 1.0);
}