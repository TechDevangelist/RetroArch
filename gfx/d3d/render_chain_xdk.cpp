/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2015 - Daniel De Matteis
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <retro_inline.h>
#include "render_chain_driver.h"

typedef struct xdk_renderchain
{
   void *empty;
} xdk_renderchain_t;

static void renderchain_set_mvp(void *data, unsigned vp_width,
      unsigned vp_height, unsigned rotation)
{
   d3d_video_t      *d3d = (d3d_video_t*)data;
   LPDIRECT3DDEVICE d3dr = (LPDIRECT3DDEVICE)d3d->dev;

#if defined(_XBOX360) && defined(HAVE_HLSL)
   hlsl_set_proj_matrix(XMMatrixRotationZ(rotation * (M_PI / 2.0)));
   if (d3d->shader && d3d->shader->set_mvp)
      d3d->shader->set_mvp(d3d, NULL);
#elif defined(HAVE_D3D8)
   D3DXMATRIX p_out, p_rotate, mat;
   D3DXMatrixOrthoOffCenterLH(&mat, 0, vp_width,  vp_height, 0, 0.0f, 1.0f);
   D3DXMatrixIdentity(&p_out);
   D3DXMatrixRotationZ(&p_rotate, rotation * (M_PI / 2.0));

   d3d_set_transform(d3dr, D3DTS_WORLD, &p_rotate);
   d3d_set_transform(d3dr, D3DTS_VIEW, &p_out);
   d3d_set_transform(d3dr, D3DTS_PROJECTION, &p_out);
#endif
}

static void renderchain_clear(void *data)
{
   d3d_video_t *d3d = (d3d_video_t*)data;

   d3d_texture_free(d3d->tex);
   d3d_vertex_buffer_free(d3d->vertex_buf, d3d->vertex_decl);
}

static bool xdk_renderchain_init_shader_fvf(void *data, void *pass_data)
{
   d3d_video_t *chain    = (d3d_video_t*)data;
   d3d_video_t *pass     = (d3d_video_t*)data;
   LPDIRECT3DDEVICE d3dr = (LPDIRECT3DDEVICE)chain->dev;

   (void)pass_data;

#if defined(_XBOX360)
   static const D3DVERTEXELEMENT VertexElements[] =
   {
      { 0, 0 * sizeof(float), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
      { 0, 2 * sizeof(float), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
      D3DDECL_END()
   };

   if (FAILED(d3dr->CreateVertexDeclaration(VertexElements, &pass->vertex_decl)))
      return false;
#endif

   return true;
}

static bool renderchain_create_first_pass(void *data,
      const video_info_t *info)
{
   d3d_video_t *chain    = (d3d_video_t*)data;
   LPDIRECT3DDEVICE d3dr = (LPDIRECT3DDEVICE)chain->dev;

   chain->vertex_buf     = d3d_vertex_buffer_new(d3dr, 4 * sizeof(Vertex), 
         D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, 
         NULL);

   if (!chain->vertex_buf)
      return false;

   chain->tex = (LPDIRECT3DTEXTURE)d3d_texture_new(d3dr, NULL, 
         chain->tex_w, chain->tex_h, 1, 0,
         info->rgb32 ? D3DFMT_LIN_X8R8G8B8 : D3DFMT_LIN_R5G6B5,
         0, 0, 0, 0, NULL, NULL);

   if (!chain->tex)
      return false;

   d3d_set_sampler_address_u(d3dr, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
   d3d_set_sampler_address_v(d3dr, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
#ifdef _XBOX1
   d3dr->SetRenderState(D3DRS_LIGHTING, FALSE);
#endif
   d3dr->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
   d3dr->SetRenderState(D3DRS_ZENABLE, FALSE);

   if (!xdk_renderchain_init_shader_fvf(chain, chain))
      return false;

   return true;
}

static void renderchain_set_vertices(void *data, unsigned pass,
      unsigned width, unsigned height)
{
   d3d_video_t *d3d    = (d3d_video_t*)data;
   runloop_t *runloop  = rarch_main_get_ptr();

   if (d3d->last_width != width || d3d->last_height != height)
   {
      unsigned i;
      Vertex vert[4];
      void *verts      = NULL;

      d3d->last_width  = width;
      d3d->last_height = height;

      float tex_w      = width;
      float tex_h      = height;
#ifdef _XBOX360
      tex_w           /= ((float)d3d->tex_w);
      tex_h           /= ((float)d3d->tex_h);
#endif

      vert[0].x        = -1.0f;
      vert[1].x        =  1.0f;
      vert[2].x        = -1.0f;
      vert[3].x        =  1.0f;

      vert[0].y        = -1.0f;
      vert[1].y        = -1.0f;
      vert[2].y        =  1.0f;
      vert[3].y        =  1.0f;
#if defined(_XBOX1)
      vert[0].z        =  1.0f;
      vert[1].z        =  1.0f;
      vert[2].z        =  1.0f;
      vert[3].z        =  1.0f;

      vert[0].rhw      = 0.0f;
      vert[1].rhw      = tex_w;
      vert[2].rhw      = 0.0f;
      vert[3].rhw      = tex_w;

      vert[0].u        = tex_h;
      vert[1].u        = tex_h;
      vert[2].u        = 0.0f;
      vert[3].u        = 0.0f;

      vert[0].v        = 0.0f;
      vert[1].v        = 0.0f;
      vert[2].v        = 0.0f;
      vert[3].v        = 0.0f;
#elif defined(_XBOX360)
      vert[0].u        = 0.0f;
      vert[1].u        = tex_w;
      vert[2].u        = 0.0f;
      vert[3].u        = tex_w;

      vert[0].v        = tex_h;
      vert[1].v        = tex_h;
      vert[2].v        = 0.0f;
      vert[3].v        = 0.0f;
#endif

      /* Align texels and vertices. */
      for (i = 0; i < 4; i++)
      {
         vert[i].x    -= 0.5f / ((float)d3d->tex_w);
         vert[i].y    += 0.5f / ((float)d3d->tex_h);
      }

      verts = d3d_vertex_buffer_lock(d3d->vertex_buf);
      memcpy(verts, vert, sizeof(vert));
      d3d_vertex_buffer_unlock(d3d->vertex_buf);
   }

#if defined(HAVE_CG) || defined(HAVE_GLSL) || defined(HAVE_HLSL)
#ifdef _XBOX
   if (d3d->shader)
   {
      renderchain_set_mvp(d3d, d3d->screen_width, d3d->screen_height, d3d->dev_rotation);
      if (d3d->shader->use)
         d3d->shader->use(d3d, pass);
      if (d3d->shader->set_params)
         d3d->shader->set_params(d3d, width, height, d3d->tex_w,
               d3d->tex_h, d3d->screen_width,
               d3d->screen_height, runloop->frames.video.count,
               NULL, NULL, NULL, 0);
   }
#endif
#endif
}

static void renderchain_blit_to_texture(void *data, const void *frame,
   unsigned width, unsigned height, unsigned pitch)
{
   D3DLOCKED_RECT d3dlr;
   d3d_video_t *d3d      = (d3d_video_t*)data;
   LPDIRECT3DDEVICE d3dr = (LPDIRECT3DDEVICE)d3d->dev;
   driver_t *driver      = driver_get_ptr();
   global_t *global      = global_get_ptr();

#if defined(_XBOX1)
   d3dr->SetFlickerFilter(global->console.screen.flicker_filter_index);
   d3dr->SetSoftDisplayFilter(global->console.softfilter_enable);
#endif

   if (d3d->last_width != width || d3d->last_height != height)
   {
      d3d_lockrectangle_clear(d3d, d3d->tex,
            0, &d3dlr, NULL, 0 /* FIXME - stub */, D3DLOCK_NOSYSLOCK);
   }

   d3d_texture_blit(driver->video_data, 0, d3d->tex,
         &d3dlr, frame, width, height, pitch);
}

static void xdk_renderchain_free(void *data)
{
   d3d_video_t *chain = (d3d_video_t*)data;

   if (!chain)
      return;

   renderchain_clear(chain);
#ifndef DONT_HAVE_STATE_TRACKER
#ifndef _XBOX
   if (chain->tracker)
      state_tracker_free(chain->tracker);
#endif
#endif
}

void xdk_renderchain_deinit(void *data)
{
   xdk_renderchain_t *renderchain = (xdk_renderchain_t*)data;

   if (renderchain)
      free(renderchain);
}

void *xdk_renderchain_new(void)
{
   xdk_renderchain_t *renderchain = (xdk_renderchain_t*)calloc(1, sizeof(*renderchain));
   if (!renderchain)
      return NULL;

   return renderchain;
}

void xdk_renderchain_deinit_shader(void)
{
   /* stub */
}

static bool xdk_renderchain_init_shader(void *data)
{
   const char *shader_path = NULL;
   d3d_video_t        *d3d = (d3d_video_t*)data;
   settings_t *settings    = config_get_ptr();

   if (!d3d)
      return false;

#if defined(HAVE_HLSL)
   RARCH_LOG("D3D]: Using HLSL shader backend.\n");
   shader_path = settings->video.shader_path;
   d3d->shader = &hlsl_backend;

   if (!d3d->shader)
      return false;

   return d3d->shader->init(d3d, shader_path);
#endif

   return true;
}

static bool xdk_renderchain_init(void *data,
      const video_info_t *info,
      void *dev_data,
      const void *final_viewport_data,
      const void *info_data,
      unsigned fmt
      )
{
   d3d_video_t *chain    = (d3d_video_t*)data;
   LPDIRECT3DDEVICE d3dr = (LPDIRECT3DDEVICE)chain->dev;
   global_t *global      = global_get_ptr();

   (void)dev_data;
   (void)final_viewport_data;
   (void)info_data;
   (void)fmt;

   chain->pixel_size     = fmt ? sizeof(uint32_t) : sizeof(uint16_t);

   if (!renderchain_create_first_pass(chain, info))
      return false;

   if (global->console.screen.viewports.custom_vp.width == 0)
      global->console.screen.viewports.custom_vp.width = chain->screen_width;

   if (global->console.screen.viewports.custom_vp.height == 0)
      global->console.screen.viewports.custom_vp.height = chain->screen_height;

   return true;
}

static void xdk_renderchain_set_final_viewport(void *data,
      void *renderchain_data, const void *viewport_data)
{
   (void)data;
   (void)renderchain_data;
   (void)viewport_data;

   /* stub */
}

static bool xdk_renderchain_render(void *data, const void *frame,
      unsigned width, unsigned height, unsigned pitch, unsigned rotation)
{
   unsigned i;
   d3d_video_t      *d3d = (d3d_video_t*)data;
   LPDIRECT3DDEVICE d3dr = (LPDIRECT3DDEVICE)d3d->dev;
   runloop_t *runloop    = rarch_main_get_ptr();
   settings_t *settings  = config_get_ptr();

   renderchain_blit_to_texture(d3d, frame, width, height, pitch);
   renderchain_set_vertices(d3d, 1, width, height);

   d3d_set_texture(d3dr, 0, d3d->tex);
   d3d_set_viewport(d3d->dev, &d3d->final_viewport);
   d3d_set_sampler_minfilter(d3dr, 0, settings->video.smooth ?
         D3DTEXF_LINEAR : D3DTEXF_POINT);
   d3d_set_sampler_magfilter(d3dr, 0, settings->video.smooth ?
         D3DTEXF_LINEAR : D3DTEXF_POINT);

#if defined(_XBOX1)
   d3d_set_vertex_shader(d3dr, D3DFVF_XYZ | D3DFVF_TEX1, NULL);
#elif defined(_XBOX360)
   D3DDevice_SetVertexDeclaration(d3dr, d3d->vertex_decl);
#endif
   for (i = 0; i < 4; i++)
      d3d_set_stream_source(d3dr, i, d3d->vertex_buf, 0, sizeof(Vertex));

   d3d_draw_primitive(d3dr, D3DPT_TRIANGLESTRIP, 0, 2);
   renderchain_set_mvp(d3d, d3d->screen_width, d3d->screen_height, d3d->dev_rotation);

   return true;
}

static bool xdk_renderchain_add_lut(void *data,
      const char *id, const char *path, bool smooth)
{
   xdk_renderchain_t *chain = (xdk_renderchain_t*)data;

   (void)data;
   (void)id;
   (void)path;
   (void)smooth;

   /* stub */
   return true;
}

static bool xdk_renderchain_add_pass(void *data, const void *info_data)
{
   (void)data;
   (void)info_data;

   /* stub */
   return true;
}

static void xdk_renderchain_add_state_tracker(void *data, void *tracker_data)
{
   (void)data;
   (void)tracker_data;

   /* stub */
}

static void xdk_renderchain_convert_geometry(
	  void *data, const void *info_data,
      unsigned *out_width, unsigned *out_height,
      unsigned width, unsigned height,
      void *final_viewport_data)
{
   (void)data;
   (void)info_data;
   (void)out_width;
   (void)out_height;
   (void)width;
   (void)height;
   (void)final_viewport_data;
   
   /* stub */
}

renderchain_driver_t xdk_renderchain = {
   xdk_renderchain_free,
   xdk_renderchain_new,
   xdk_renderchain_deinit,
   xdk_renderchain_deinit_shader,
   xdk_renderchain_init_shader,
   xdk_renderchain_init_shader_fvf,
   xdk_renderchain_init,
   xdk_renderchain_set_final_viewport,
   xdk_renderchain_add_pass,
   xdk_renderchain_add_lut,
   xdk_renderchain_add_state_tracker,
   xdk_renderchain_render,
   xdk_renderchain_convert_geometry,
   "xdk",
};
