#ifndef D3DU_H
#define D3DU_H

// you need to include windows.h and d3d11.h first.

struct d3du_context {
   HWND hwnd;
   ID3D11Device * dev;
   ID3D11DeviceContext * ctx;
   IDXGISwapChain * swap;

   ID3D11Texture2D * backbuf;
   ID3D11Texture2D * depthbuf;

   ID3D11RenderTargetView * backbuf_rtv;
   ID3D11DepthStencilView * depthbuf_dsv;

   D3D11_VIEWPORT default_vp;
};

// Creates a D3DU context and opens a window with given title and width/height
d3du_context * d3du_init( char const * title, int w, int h, D3D_FEATURE_LEVEL feature_level );

// Shuts down a D3DU context and frees it.
void d3du_shutdown( d3du_context * ctx );

// Processes window events. Returns 1 if OK, 0 if user requested exit.
int d3du_handle_events( d3du_context * ctx );

// Swap buffers.
void d3du_swap_buffers( d3du_context * ctx, bool vsync );

// Creates a D3D11_VIEWPORT for an entire render target view.
D3D11_VIEWPORT d3du_full_tex2d_viewport( ID3D11Texture2D * tex );

// Creates a buffer
ID3D11Buffer * d3du_make_buffer( ID3D11Device * dev, UINT size, D3D11_USAGE use, UINT bind_flags, const void * initial );

// Reads back the contents of a buffer and returns them as an unsigned char array.
// size_in_bytes, when non-NULL, will receive the buffer size.
//
// Intended for debugging only.
unsigned char * d3du_get_buffer( d3du_context * ctx, ID3D11Buffer * buf, int * size_in_bytes );

// Reads back the contents of the given mip level of a texture SRV.
//
// Intended for debugging only.
unsigned char * d3du_read_texture_level( d3du_context * ctx, ID3D11ShaderResourceView * srv, int level );

// Creates a simple rasterizer state
ID3D11RasterizerState * d3du_simple_raster( ID3D11Device * dev, D3D11_CULL_MODE cull, bool front_ccw, bool scissor_enable );

// Creates a simple blend state.
ID3D11BlendState * d3du_simple_blend( ID3D11Device * dev, D3D11_BLEND src_blend, D3D11_BLEND dest_blend );

// Creates a simplified sampler state.
ID3D11SamplerState * d3du_simple_sampler( ID3D11Device * dev, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addr );

// Compiles the given shader or dies trying!
ID3DBlob * d3du_compile_source_or_die( char const * source, char const * profile, char const * entrypt );

// *All* the shaders.
union d3du_shader {
   ID3D11DeviceChild * generic;
   ID3D11PixelShader * ps;
   ID3D11VertexShader * vs;
   ID3D11ComputeShader * cs;
};

// Compile and create a shader with the given profile on the given device
d3du_shader d3du_compile_and_create_shader( ID3D11Device * dev, char const * source, char const * profile, char const * entrypt );

// Texture helper
struct d3du_tex {
    union {
        ID3D11Resource * resrc;
        ID3D11Texture2D * tex2d;
        ID3D11Texture2D * tex3d;
    };
    ID3D11ShaderResourceView * srv;
    ID3D11RenderTargetView * rtv;

    ~d3du_tex();

    static d3du_tex * make2d( ID3D11Device * dev, UINT w, UINT h, UINT num_mips,
        DXGI_FORMAT fmt, D3D11_USAGE usage, UINT bind_flags, void const * initial, UINT initial_pitch );

    static d3du_tex * make3d( ID3D11Device * dev, UINT w, UINT h, UINT d, UINT num_mips,
        DXGI_FORMAT fmt, D3D11_USAGE usage, UINT bind_flags, void const * initial, UINT init_row_pitch, UINT init_depth_pitch );

private:
    d3du_tex( ID3D11Resource * resrc, ID3D11ShaderResourceView * srv, ID3D11RenderTargetView * rtv );
};

// D3DU timer measures how long D3D calls take on the GPU side
// Create, call bracket begin/end around area you want to capture, then "report" at the end.
typedef struct d3du_timer d3du_timer;

d3du_timer * d3du_timer_create( d3du_context * ctx, size_t warmup_frames ); // warmup_frames = no. of initial measurements to throw away
void d3du_timer_destroy( d3du_timer * timer );
void d3du_timer_bracket_begin( d3du_context * ctx, d3du_timer * timer );
void d3du_timer_bracket_end( d3du_context * ctx, d3du_timer * timer );
void d3du_timer_report( d3du_context * ctx, d3du_timer * timer, char const * label );

#endif

