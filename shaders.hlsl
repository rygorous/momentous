#define TEX_WIDTH_LOG2 10

struct CubeVert {
    float4 clip_pos : SV_Position;
    float3 world_pos : WorldPos; // .xyz = world space position
};

cbuffer CubeConsts : register(b0) {
    float4x4 clip_from_world;
    float3 world_down_vector;
    float time_offs;

    // diffuse trilight plus ambient
    float3 light_color_ambient;
    float3 light_color_key;
    float3 light_color_fill;
    float3 light_color_back;
    float3 light_dir;
};

cbuffer UpdateConsts : register(b1) {
    float3 field_scale;
    float  damping;
    float3 field_offs;
    float  accel;
    float3 field_sample_scale;
    float  vel_scale;
};

float4 UpdateVertShader(
    uint vertex_id : SV_VertexID
) : SV_Position
{
    return float4(float(vertex_id >> 1) * 4.0 - 1.0, 1.0 - float(vertex_id & 1) * 4.0, 0.5, 1.0);
}

float4 UpdatePosShader(
    float4 pos : SV_Position,
    SamplerState force_smp : register(s0),
    Texture2D tex_older_pos : register(t0),
    Texture2D tex_newer_pos : register(t1),
    Texture3D tex_force : register(t2)
) : SV_Target
{
    int3 coord_pos = int3(int2(pos.xy), 0);
    float4 older_pos = tex_older_pos.Load(coord_pos);
    float4 newer_pos = tex_newer_pos.Load(coord_pos);

    // determine force field sample pos
    float3 force_pos = newer_pos.xyz * field_scale + field_offs;
    float3 force_frac = frac(force_pos);
    float3 force_smooth = force_frac * force_frac * (3.0 - 2.0 * force_frac);
    force_pos = (force_pos - force_frac) + force_smooth;

    // sample force from texture
    float3 force = tex_force.Sample(force_smp, force_pos * field_sample_scale).xyz;

    // verlet integration
    float3 new_pos = newer_pos.xyz + damping * (newer_pos.xyz - older_pos.xyz);
    new_pos += accel * force;

    float4 output = float4(new_pos, newer_pos.w);

    // nuke particles if they get too far from the origin
    if (dot(new_pos, new_pos) > 16.0)
        output.w = 0.0;

    return output;
}

float4 UpdateVelShader(
    float4 pos : SV_Position,
    Texture2D tex_older_pos : register(t0),
    Texture2D tex_newer_pos : register(t1)
) : SV_Target
{
    int3 coord_pos = int3(int2(pos.xy), 0);
    float4 older_pos = tex_older_pos.Load(coord_pos);
    float4 newer_pos = tex_newer_pos.Load(coord_pos);

    return newer_pos - older_pos;
}

CubeVert RenderCubeVertexShader(
    uint vertex_id : SV_VertexID,
    uint instance_id : SV_InstanceID,
    Texture2D tex_pos : register(t0),
    Texture2D tex_fwd : register(t1)
)
{
    CubeVert v;

    // fetch cube position and velocity from textures
    int3 fetch_coord = int3(vertex_id >> 3, instance_id, 0);
    float4 cube_pos = tex_pos.Load(fetch_coord);
    float4 cube_fwd = tex_fwd.Load(fetch_coord);

    // early-out if cube is off
    if (cube_pos.w == 0.0) {
        v.clip_pos = 0;
        v.world_pos = 0;
        return v;
    }

    // determine local coordinate system
    float3 x_axis = cube_fwd.xyz;
    float3 z_axis = normalize(cross(x_axis, world_down_vector));
    float3 y_axis = normalize(cross(z_axis, x_axis));

    // generate cube vertex
    float3 world_pos = cube_pos.xyz;
    float across_size = cube_pos.w;

    world_pos += (((vertex_id & 1) != 0) ? 1.0 : -1.0) * x_axis;
    world_pos += (((vertex_id & 2) != 0) ? across_size : -across_size) * y_axis;
    world_pos += (((vertex_id & 4) != 0) ? across_size : -across_size) * z_axis;

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