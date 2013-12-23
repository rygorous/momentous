#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <string.h>
#include "d3du.h"
#include "util.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

static LRESULT CALLBACK window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    //d3du_context * ctx = (glx_context *) GetWindowLongPtrA( hwnd, GWLP_USERDATA );

    switch ( msg )
    {
    case WM_CREATE:
        {
            CREATESTRUCTA * cs = (CREATESTRUCTA *) lparam;
            SetWindowLongPtrA( hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams );
        }
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        ValidateRect( hwnd, NULL );
        return 0;

    case WM_CHAR:
        if ( wparam == 27 ) // escape
            PostMessage( hwnd, WM_CLOSE, 0, 0 );
        return 0;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;
    }

    return DefWindowProcA( hwnd, msg, wparam, lparam );
}

template<typename T>
static void safe_release( T * * p )
{
    if ( *p )
    {
        (*p)->Release();
        *p = NULL;
    }
}

static d3du_context * d3du_init_fail( d3du_context * ctx )
{
    safe_release( &ctx->backbuf );
    safe_release( &ctx->depthbuf );
    safe_release( &ctx->backbuf_rtv );
    safe_release( &ctx->depthbuf_dsv );
    safe_release( &ctx->swap );
    safe_release( &ctx->ctx );
    safe_release( &ctx->dev );
    if ( ctx->hwnd ) DestroyWindow( ctx->hwnd );
    delete ctx;
    return NULL;
}

d3du_context * d3du_init( char const * title, int w, int h, D3D_FEATURE_LEVEL feature_level )
{
    d3du_context * ctx = new d3du_context;
    memset( ctx, 0, sizeof( *ctx ) );

    HINSTANCE hinst = GetModuleHandleA( NULL );

    WNDCLASSA wc = { 0 };
    wc.hbrBackground = (HBRUSH) GetStockObject( BLACK_BRUSH );
    wc.hCursor = LoadCursor( 0, IDC_ARROW );
    wc.hInstance = hinst;
    wc.lpfnWndProc = window_proc;
    wc.lpszClassName = "rad.d3du";
    RegisterClassA( &wc );

    DWORD style = WS_OVERLAPPEDWINDOW;

    RECT rc = { 0, 0, w, h };
    AdjustWindowRect( &rc, style, FALSE );

    ctx->hwnd = CreateWindowExA( 0, "rad.d3du", title, style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hinst, ctx );
    if ( !ctx->hwnd )
        return d3du_init_fail( ctx );

    DXGI_SWAP_CHAIN_DESC swap_desc = { 0 };
    swap_desc.BufferDesc.Width = w;
    swap_desc.BufferDesc.Height = h;
    swap_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    swap_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swap_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swap_desc.SampleDesc.Count = 1;
    swap_desc.SampleDesc.Quality = 0;
    swap_desc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.BufferCount = 1;
    swap_desc.OutputWindow = ctx->hwnd;
    swap_desc.Windowed = TRUE;
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swap_desc.Flags = 0;

    D3D_FEATURE_LEVEL out_level;
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    HRESULT hr = D3D11CreateDeviceAndSwapChain( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags,
        &feature_level, 1, D3D11_SDK_VERSION, &swap_desc, &ctx->swap, &ctx->dev, &out_level, &ctx->ctx );
    if ( FAILED( hr ) )
        return d3du_init_fail( ctx );

    // render target and rtv
    hr = ctx->swap->GetBuffer( 0, __uuidof(ID3D11Texture2D), (void **)&ctx->backbuf );
    if ( FAILED( hr ) )
        return d3du_init_fail( ctx );

    hr = ctx->dev->CreateRenderTargetView( ctx->backbuf, NULL, &ctx->backbuf_rtv );
    if ( FAILED( hr ) )
        return d3du_init_fail( ctx );

    // depth/stencil surface and dsv
    D3D11_TEXTURE2D_DESC desc =
    {
        w, h, 1, 1, DXGI_FORMAT_D32_FLOAT_S8X24_UINT, { 1, 0 },
        D3D11_USAGE_DEFAULT, D3D11_BIND_DEPTH_STENCIL, 0, 0
    };
    hr = ctx->dev->CreateTexture2D( &desc, NULL, &ctx->depthbuf );
    if ( FAILED( hr ) )
        return d3du_init_fail( ctx );
    
    hr = ctx->dev->CreateDepthStencilView( ctx->depthbuf, NULL, &ctx->depthbuf_dsv );
    if ( FAILED( hr ) )
        return d3du_init_fail( ctx );

    ctx->default_vp.TopLeftX = 0.0f;
    ctx->default_vp.TopLeftY = 0.0f;
    ctx->default_vp.Width = (float)w;
    ctx->default_vp.Height = (float)h;
    ctx->default_vp.MinDepth = 0.0f;
    ctx->default_vp.MaxDepth = 1.0f;

    // bind default RT, DSV and viewport for convenience.
    ctx->ctx->OMSetRenderTargets( 1, &ctx->backbuf_rtv, ctx->depthbuf_dsv );
    ctx->ctx->RSSetViewports( 1, &ctx->default_vp );

    return ctx;
}

void d3du_shutdown( d3du_context * ctx )
{
    if ( ctx->ctx )
        ctx->ctx->ClearState();

    safe_release( &ctx->backbuf );
    safe_release( &ctx->depthbuf );
    safe_release( &ctx->backbuf_rtv );
    safe_release( &ctx->depthbuf_dsv );
    safe_release( &ctx->swap );
    safe_release( &ctx->ctx );

#if 0 && defined(_DEBUG) // use to trace leaks
    if ( ctx->dev )
    {
        ID3D11Debug * dbg;
        ctx->dev->QueryInterface( __uuidof(ID3D11Debug), (void**)&dbg );
        dbg->ReportLiveDeviceObjects( D3D11_RLDO_DETAIL );
        dbg->Release();
    }
#endif

    safe_release( &ctx->dev );
    DestroyWindow( ctx->hwnd );
    delete ctx;
}

int d3du_handle_events( d3du_context * ctx )
{
    MSG msg;
    int ok = 1;

    while ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
    {
        if ( msg.message == WM_QUIT )
            ok = 0;
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    return ok;
}

void d3du_swap_buffers( d3du_context * ctx, bool vsync )
{
   ctx->swap->Present( vsync ? 1 : 0, 0 );
}

D3D11_VIEWPORT d3du_full_tex2d_viewport( ID3D11Texture2D * tex )
{
    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc( &desc );

    D3D11_VIEWPORT vp;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = (float)desc.Width;
    vp.Height = (float)desc.Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    return vp;
}

ID3D11Buffer * d3du_make_buffer( ID3D11Device * dev, UINT size, D3D11_USAGE use, UINT bind_flags, const void * initial )
{
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = size;
    desc.Usage = use;
    desc.BindFlags = bind_flags;
    switch ( use )
    {
    case D3D11_USAGE_DYNAMIC:
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        break;

    case D3D11_USAGE_STAGING:
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
        break;

    default:
        desc.CPUAccessFlags = 0;
        break;
    }
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initial_data;
    initial_data.pSysMem = initial;
    initial_data.SysMemPitch = 0;
    initial_data.SysMemSlicePitch = 0;

    ID3D11Buffer * buf;
    HRESULT hr = dev->CreateBuffer( &desc, initial ? &initial_data : NULL, &buf );
    if ( FAILED( hr ) )
        panic( "D3D CreateBuffer failed: 0x%08x\n", hr );

    return buf;
}

unsigned char * d3du_get_buffer( d3du_context * ctx, ID3D11Buffer * buf, int * size_in_bytes )
{
    D3D11_BUFFER_DESC desc;
    buf->GetDesc( &desc );
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    ID3D11Buffer * temp_buf;
    HRESULT hr = ctx->dev->CreateBuffer( &desc, NULL, &temp_buf );
    if ( FAILED( hr ) )
        return NULL;

    ctx->ctx->CopyResource( temp_buf, buf );

    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = ctx->ctx->Map( temp_buf, 0, D3D11_MAP_READ, 0, &mapped );
    if ( FAILED( hr ) )
        panic( "d3du_get_buffer map failed\n" );

    unsigned char * result = new unsigned char[desc.ByteWidth];
    memcpy( result, mapped.pData, desc.ByteWidth );

    ctx->ctx->Unmap( temp_buf, 0 );
    temp_buf->Release();

    if ( size_in_bytes )
        *size_in_bytes = desc.ByteWidth;

    return result;
}

static unsigned int get_bpp( DXGI_FORMAT fmt )
{
    unsigned int bpp = 0;

    switch ( fmt )
    {
    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
        bpp = 1;
        break;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
        bpp = 2;
        break;

    default:
        panic( "unsupported DXGI format %d\n", fmt );
    }

    return bpp;
}

unsigned char * d3du_read_texture_level( d3du_context * ctx, ID3D11ShaderResourceView * srv, int srv_level )
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    srv->GetDesc( &srv_desc );

    if ( srv_desc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2D )
        panic( "d3du_read_texture_level only supports 2D textures right now" );

    unsigned int bpp = get_bpp( srv_desc.Format );
    int res_level = srv_level + srv_desc.Texture2D.MostDetailedMip;

    D3D11_TEXTURE2D_DESC tex_desc;
    ID3D11Texture2D * tex2d;
    srv->GetResource( (ID3D11Resource **)&tex2d );
    tex2d->GetDesc( &tex_desc );

    tex_desc.Usage = D3D11_USAGE_STAGING;
    tex_desc.BindFlags = 0;
    tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    tex_desc.MiscFlags = 0;

    ID3D11Texture2D * temp_tex;
    HRESULT hr = ctx->dev->CreateTexture2D( &tex_desc, NULL, &temp_tex );
    if ( FAILED( hr ) )
    {
        tex2d->Release();
        return NULL;
    }

    ctx->ctx->CopyResource( temp_tex, tex2d );

    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = ctx->ctx->Map( temp_tex, res_level, D3D11_MAP_READ, 0, &mapped );
    if ( FAILED( hr ) )
        panic( "d3du_read_texture_level map failed\n" );

    unsigned int out_width = tex_desc.Width >> res_level;
    unsigned int out_height = tex_desc.Height >> res_level;

    if ( !out_width ) out_width = 1;
    if ( !out_height ) out_height = 1;

    unsigned int out_pitch = out_width * bpp;

    unsigned char * result = new unsigned char[out_pitch * out_height];
    for ( unsigned int y = 0 ; y < out_height ; y++ )
        memcpy( result + y*out_pitch, (unsigned char *)mapped.pData + y*mapped.RowPitch, out_pitch );

    ctx->ctx->Unmap( temp_tex, res_level );
    temp_tex->Release();
    tex2d->Release();

    return result;
}

ID3D11RasterizerState * d3du_simple_raster( ID3D11Device * dev, D3D11_CULL_MODE cull, bool front_ccw, bool scissor_enable )
{
    D3D11_RASTERIZER_DESC raster_desc = { D3D11_FILL_SOLID };
    raster_desc.CullMode = cull;
    raster_desc.FrontCounterClockwise = front_ccw;
    raster_desc.DepthClipEnable = TRUE;
    raster_desc.ScissorEnable = scissor_enable;

    ID3D11RasterizerState * raster_state = NULL;
    HRESULT hr = dev->CreateRasterizerState( &raster_desc, &raster_state );
    if ( FAILED( hr ) )
        panic( "CreateRasterizerState failed\n" );

    return raster_state;
}

ID3D11BlendState * d3du_simple_blend( ID3D11Device * dev, D3D11_BLEND src_blend, D3D11_BLEND dest_blend )
{
    D3D11_BLEND_DESC blend_desc = { FALSE, FALSE };
    blend_desc.RenderTarget[0].BlendEnable = ( src_blend != D3D11_BLEND_ONE || dest_blend != D3D11_BLEND_ZERO );
    blend_desc.RenderTarget[0].SrcBlend = src_blend;
    blend_desc.RenderTarget[0].DestBlend = dest_blend;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = src_blend;
    blend_desc.RenderTarget[0].DestBlendAlpha = dest_blend;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ID3D11BlendState * blend_state = NULL;
    HRESULT hr = dev->CreateBlendState( &blend_desc, &blend_state );
    if ( FAILED( hr ) )
        panic( "CreateBlendState failed\n" );

    return blend_state;
}

ID3D11SamplerState * d3du_simple_sampler( ID3D11Device * dev, D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE addr )
{
    HRESULT hr;
    ID3D11SamplerState * sampler = NULL;

    D3D11_SAMPLER_DESC desc;
    desc.Filter = filter;
    desc.AddressU = addr;
    desc.AddressV = addr;
    desc.AddressW = addr;
    desc.MipLODBias = 0.0f;
    desc.MaxAnisotropy = 8;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    desc.BorderColor[0] = 1.0f;
    desc.BorderColor[1] = 1.0f;
    desc.BorderColor[2] = 1.0f;
    desc.BorderColor[3] = 1.0f;
    desc.MinLOD = -1e+20f;
    desc.MaxLOD = 1e+20f;

    hr = dev->CreateSamplerState( &desc, &sampler );
    if ( FAILED( hr ) )
        panic( "CreateSamplerState failed\n" );

    return sampler;
}

ID3DBlob * d3du_compile_source_or_die( char const * source, char const * profile, char const * entrypt )
{
    ID3DBlob * code;
    ID3DBlob * errors;
    HRESULT hr = D3DCompile( source, strlen( source ), NULL, NULL, NULL, entrypt, profile, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL1, 0,
        &code, &errors );

    if ( errors )
    {
        OutputDebugStringA( "While compiling:\n" );
        OutputDebugStringA( source );
        OutputDebugStringA( "Got errors:\n" );
        OutputDebugStringA( (char*)errors->GetBufferPointer() );
        errors->Release();
    }

    if ( FAILED( hr ) )
        panic( "Shader compilation failed!\n" );

    return code;
}

d3du_shader d3du_compile_and_create_shader( ID3D11Device * dev, char const * source, char const * profile, char const * entrypt )
{
    ID3DBlob * code = d3du_compile_source_or_die( source, profile, entrypt );
    HRESULT hr = S_OK;
    d3du_shader sh;

    sh.generic = NULL;

    switch ( profile[0] )
    {
    case 'p':   hr = dev->CreatePixelShader( code->GetBufferPointer(), code->GetBufferSize(), NULL, &sh.ps ); break;
    case 'v':   hr = dev->CreateVertexShader( code->GetBufferPointer(), code->GetBufferSize(), NULL, &sh.vs ); break;
    case 'c':   hr = dev->CreateComputeShader( code->GetBufferPointer(), code->GetBufferSize(), NULL, &sh.cs ); break;
    default:    panic( "Unsupported shader profile '%s'\n", profile );
    }

    if ( FAILED( hr ) )
        panic( "Error creating shader.\n" );

    return sh;
}

d3du_tex::d3du_tex( ID3D11Resource * resrc, ID3D11ShaderResourceView * srv, ID3D11RenderTargetView * rtv )
    : resrc(resrc), srv(srv), rtv(rtv)
{
}

d3du_tex::~d3du_tex()
{
    safe_release( &resrc );
    safe_release( &srv );
    safe_release( &rtv );
}

d3du_tex * d3du_tex::make2d( ID3D11Device * dev, UINT w, UINT h, UINT num_mips, DXGI_FORMAT fmt, D3D11_USAGE usage, UINT bind_flags, void const * initial, UINT initial_pitch )
{
    HRESULT hr = S_OK;
    ID3D11Texture2D *tex = NULL;
    ID3D11ShaderResourceView *srv = NULL;
    ID3D11RenderTargetView *rtv = NULL;

    D3D11_TEXTURE2D_DESC desc;
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = num_mips;
    desc.ArraySize = 1;
    desc.Format = fmt;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = usage;
    desc.BindFlags = bind_flags;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initial_data;
    initial_data.pSysMem = initial;
    initial_data.SysMemPitch = initial_pitch;
    initial_data.SysMemSlicePitch = 0;

    hr = dev->CreateTexture2D( &desc, initial ? &initial_data : nullptr, &tex );

    if ( !FAILED( hr ) && ( bind_flags & D3D11_BIND_SHADER_RESOURCE ) )
        hr = dev->CreateShaderResourceView( tex, nullptr, &srv );

    if ( !FAILED( hr ) && ( bind_flags & D3D11_BIND_RENDER_TARGET ) )
        hr = dev->CreateRenderTargetView( tex, nullptr, &rtv );

    if ( FAILED( hr ) )
    {
        safe_release( &tex );
        safe_release( &srv );
        safe_release( &rtv );
        return NULL;
    } else
        return new d3du_tex( tex, srv, rtv );
}

d3du_tex * d3du_tex::make3d( ID3D11Device * dev, UINT w, UINT h, UINT d, UINT num_mips, DXGI_FORMAT fmt, D3D11_USAGE usage, UINT bind_flags, void const * initial, UINT init_row_pitch, UINT init_depth_pitch )
{
    HRESULT hr = S_OK;
    ID3D11Texture3D *tex = NULL;
    ID3D11ShaderResourceView *srv = NULL;

    D3D11_TEXTURE3D_DESC desc;
    desc.Width = w;
    desc.Height = h;
    desc.Depth = d;
    desc.MipLevels = num_mips;
    desc.Format = fmt;
    desc.Usage = usage;
    desc.BindFlags = bind_flags;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initial_data;
    initial_data.pSysMem = initial;
    initial_data.SysMemPitch = init_row_pitch;
    initial_data.SysMemSlicePitch = init_depth_pitch;

    hr = dev->CreateTexture3D( &desc, initial ? &initial_data : nullptr, &tex );

    if ( !FAILED( hr ) && ( bind_flags & D3D11_BIND_SHADER_RESOURCE ) )
        hr = dev->CreateShaderResourceView( tex, nullptr, &srv );

    if ( FAILED( hr ) )
    {
        safe_release( &tex );
        safe_release( &srv );
        return NULL;
    } else
        return new d3du_tex( tex, srv, NULL );
}

static const size_t TIMER_SLOTS = 4; // depth of queue of in-flight queries (must be pow2)

struct d3du_timer_group
{
    ID3D11Query * begin;
    ID3D11Query * end;
    ID3D11Query * disjoint;
};

struct d3du_timer
{
    d3du_timer_group grp[TIMER_SLOTS];
    size_t issue_idx; // index of timer we're issuing
    size_t retire_idx; // index of timer we're retiring
    size_t warmup_frames;
    run_stats * stats;
};

static d3du_timer_group * timer_get( d3du_timer * timer, size_t index )
{
    return &timer->grp[ index & ( TIMER_SLOTS - 1 ) ];
}

static void timer_ensure_max_in_flight( d3du_context * ctx, d3du_timer * timer, size_t max_in_flight )
{
    while ( ( timer->issue_idx - timer->retire_idx ) > max_in_flight )
    {
        // retire oldest timer in flight
        d3du_timer_group * grp = timer_get( timer, timer->retire_idx );
        UINT64 start, end;
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;
        HRESULT hr;

        while ( ( hr = ctx->ctx->GetData( grp->begin, &start, sizeof( UINT64 ), 0 ) ) != S_OK );
        while ( ( hr = ctx->ctx->GetData( grp->end, &end, sizeof( UINT64 ), 0 ) ) != S_OK );
        while ( ( hr = ctx->ctx->GetData( grp->disjoint, &disjoint, sizeof( disjoint ), 0 ) ) != S_OK );

        if ( timer->retire_idx >= timer->warmup_frames && !disjoint.Disjoint )
            run_stats_record( timer->stats, (float) ( 1000.0 * ( end - start ) / disjoint.Frequency ) );

        timer->retire_idx++;
    }
}

d3du_timer * d3du_timer_create( d3du_context * ctx, size_t warmup_frames )
{
    d3du_timer * timer = new d3du_timer;

    for ( size_t i = 0 ; i < TIMER_SLOTS ; i++ )
    {
        D3D11_QUERY_DESC desc = {};
        HRESULT hr;
        desc.Query = D3D11_QUERY_TIMESTAMP;
        hr = ctx->dev->CreateQuery( &desc, &timer->grp[i].begin );
        if ( FAILED( hr ) ) panic( "CreateQuery failed.\n" );
        hr = ctx->dev->CreateQuery( &desc, &timer->grp[i].end );
        if ( FAILED( hr ) ) panic( "CreateQuery failed.\n" );

        desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        hr = ctx->dev->CreateQuery( &desc, &timer->grp[i].disjoint );
        if ( FAILED( hr ) ) panic( "CreateQuery failed.\n" );
    }

    timer->issue_idx = 0;
    timer->retire_idx = 0;
    timer->warmup_frames = warmup_frames;
    timer->stats = run_stats_create();
    return timer;
}

void d3du_timer_destroy( d3du_timer * timer )
{
    if ( timer )
    {
        for ( size_t i = 0 ; i < TIMER_SLOTS ; i++ )
        {
            safe_release( &timer->grp[i].begin );
            safe_release( &timer->grp[i].end );
            safe_release( &timer->grp[i].disjoint );
        }

        run_stats_destroy( timer->stats );
        delete timer;
    }
}

void d3du_timer_bracket_begin( d3du_context * ctx, d3du_timer * timer )
{
    // make sure we have a free timer to issue first
    timer_ensure_max_in_flight( ctx, timer, TIMER_SLOTS - 1 );

    d3du_timer_group * grp = timer_get( timer, timer->issue_idx );

    ctx->ctx->Begin( grp->disjoint );
    ctx->ctx->End( grp->begin );
    timer->issue_idx++;
}

void d3du_timer_bracket_end( d3du_context * ctx, d3du_timer * timer )
{
    d3du_timer_group * grp = timer_get( timer, timer->issue_idx - 1 );

    ctx->ctx->End( grp->end );
    ctx->ctx->End( grp->disjoint );
}

void d3du_timer_report( d3du_context * ctx, d3du_timer * timer, char const * label )
{
    timer_ensure_max_in_flight( ctx, timer, 0 );
    run_stats_report( timer->stats, label );
}

// @cdep pre $set(c8sysincludes, -I$dxPath/include $c8sysincludes)
// @cdep pre $set(csysincludes64EMT, -I$dxPath/include $csysincludes64EMT)

