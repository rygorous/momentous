#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <cmath>

#include "d3du.h"
#include "util.h"
#include "math.h"

struct CubeConstBuf {
    math::mat44 clip_from_world;
    math::vec3 world_down_vector;
    float cube_size;

    math::vec3 light_color_ambient;
    float pad1;
    math::vec3 light_color_key;
    float pad2;
    math::vec3 light_color_fill;
    float pad3;
    math::vec3 light_color_back;
    float pad4;
    math::vec3 light_dir;
    float pad5;
};

static float srgb2lin(float x)
{
    static const float lin_thresh = 0.04045f;
    if (x < lin_thresh)
        return x * (1.0f / 12.92f);
    else
        return std::pow((x + 0.055f) / 1.055f, 2.4f);
}

static math::vec3 srgb_color(int col)
{
    return math::vec3(
        srgb2lin(((col >> 16) & 0xff) / 255.0f),
        srgb2lin(((col >>  8) & 0xff) / 255.0f),
        srgb2lin(((col >>  0) & 0xff) / 255.0f)
    );
}

static void* map_cbuf_typeless(d3du_context *ctx, ID3D11Buffer *buf)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = ctx->ctx->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr))
        panic("D3D buffer map failed!\n");
    return mapped.pData;
}

template<typename T>
static T* map_cbuf(d3du_context *ctx, ID3D11Buffer *buf)
{
    return (T*) map_cbuf_typeless(ctx, buf);
}

static void unmap_cbuf(d3du_context *ctx, ID3D11Buffer *buf)
{
    ctx->ctx->Unmap(buf, 0);
}

int main()
{
    d3du_context *d3d = d3du_init("Momentous", 1280, 720, D3D_FEATURE_LEVEL_10_0);

    char *shader_source = read_file("shaders.hlsl");

    ID3D11VertexShader *cube_vs = d3du_compile_and_create_shader(d3d->dev, shader_source,
        "vs_4_0", "RenderCubeVertexShader").vs;
    ID3D11PixelShader *cube_ps = d3du_compile_and_create_shader(d3d->dev, shader_source,
        "ps_4_0", "RenderCubePixelShader").ps;

    free(shader_source);

    static const USHORT cube_inds[] = {
        0, 2, 1, 3, 7, 2, 6, 0, 4, 1, 5, 7, 4, 6,
    };

    ID3D11Buffer *cube_const_buf = d3du_make_buffer(d3d->dev, sizeof(CubeConstBuf),
        D3D11_USAGE_DYNAMIC, D3D11_BIND_CONSTANT_BUFFER, NULL);

    ID3D11Buffer *cube_index_buf = d3du_make_buffer(d3d->dev, sizeof(cube_inds),
        D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, cube_inds);

    ID3D11RasterizerState *raster_state = d3du_simple_raster(d3d->dev, D3D11_CULL_BACK, true, false);

    int frame = 0;

    while (d3du_handle_events(d3d)) {
        using namespace math;

        static const float clear_color[4] = { 0.3f, 0.6f, 0.9f, 1.0f };
        d3d->ctx->ClearDepthStencilView(d3d->depthbuf_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        d3d->ctx->ClearRenderTargetView(d3d->backbuf_rtv, clear_color);

        // set up camera
        vec3 world_cam_pos(-2.0f, -2.0f, -5.0f);
        vec3 world_cam_target(0, 0, 0);
        world_cam_pos.y = -2.0f * cos(frame++ * 0.01f);
        mat44 view_from_world = mat44::look_at(world_cam_pos, world_cam_target, vec3(0,1,0));

        // projection
        mat44 clip_from_view = mat44::perspectiveD3D(1280.0f / 720.0f, 1.0f, 1.0f, 1000.0f);
        mat44 clip_from_world = clip_from_view * view_from_world;

        auto cube_consts = map_cbuf<CubeConstBuf>(d3d, cube_const_buf);
        cube_consts->clip_from_world = clip_from_world;
        cube_consts->world_down_vector = vec3(0, 1, 0);
        cube_consts->cube_size = 1.0f;
        cube_consts->light_color_ambient = srgb_color(0x202020);
        cube_consts->light_color_key = srgb_color(0xc0c0c0);
        cube_consts->light_color_back = srgb_color(0x101040);
        cube_consts->light_color_fill = srgb_color(0x602020);
        cube_consts->light_dir = normalize(vec3(0.0f, -0.7f, -0.3f));
        unmap_cbuf(d3d, cube_const_buf);

        // render
        d3d->ctx->VSSetShader(cube_vs, NULL, 0);
        d3d->ctx->VSSetConstantBuffers(0, 1, &cube_const_buf);

        d3d->ctx->PSSetShader(cube_ps, NULL, 0);
        d3d->ctx->PSSetConstantBuffers(0, 1, &cube_const_buf);

        d3d->ctx->RSSetState(raster_state);

        d3d->ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        d3d->ctx->IASetIndexBuffer(cube_index_buf, DXGI_FORMAT_R16_UINT, 0);
        d3d->ctx->DrawIndexed(sizeof(cube_inds)/sizeof(*cube_inds), 0, 0);

        d3du_swap_buffers(d3d, true);
    }

    cube_const_buf->Release();
    cube_index_buf->Release();
    cube_ps->Release();
    cube_vs->Release();
    raster_state->Release();

    d3du_shutdown(d3d);
    return 0;
}