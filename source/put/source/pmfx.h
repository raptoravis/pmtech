// pmfx.h
// Copyright 2014 - 2019 Alex Dixon.
// License: https://github.com/polymonster/pmtech/blob/master/license.md

#ifndef _pmfx_h
#define _pmfx_h

#include "renderer.h"
#include "str/Str.h"
#include "types.h"

namespace pen
{
    struct viewport;
}

namespace put
{
    namespace ecs
    {
        struct ecs_scene;
    }

    struct camera;
} // namespace put

namespace put
{
    enum e_update_order
    {
        PRE_UPDATE = 0,
        MAIN_UPDATE,
        POST_UPDATE,

        UPDATES_NUM
    };

    enum e_render_constants
    {
        MAX_TECHNIQUE_SAMPLER_BINDINGS = 8, // 8 samplers possibly set from shader / technique
        MAX_SAMPLER_BINDINGS = 16           // +8 samplers for gloabl stuff ie. shadow maps.
    };

    struct scene_view
    {
        u32             cb_view = PEN_INVALID_HANDLE;
        u32             cb_2d_view = PEN_INVALID_HANDLE;
        u32             render_flags = 0;
        u32             depth_stencil_state = 0;
        u32             blend_state = 0;
        u32             raster_state = 0;
        u32             array_index = 0;
        u32             num_arrays = 1;
        put::camera*    camera = nullptr;
        pen::viewport*  viewport = nullptr;
        u32             pmfx_shader = PEN_INVALID_HANDLE;
        hash_id         technique = 0; // todo rename to id_technique
        u32             permutation = 0;
        ecs::ecs_scene* scene = nullptr;
        bool            viewport_correction = false;
    };

    struct scene_view_renderer
    {
        Str     name;
        hash_id id_name = 0;

        void (*render_function)(const scene_view&) = nullptr;
    };

    struct technique_constant_data
    {
        f32 data[64] = {0};
    };

    struct sampler_binding
    {
        hash_id id_texture = 0;
        hash_id id_sampler_state = 0;
        u32     handle = 0;
        u32     sampler_unit = 0;
        u32     sampler_state = 0;
        u32     bind_flags = 0;
    };

    struct sampler_set
    {
        // 8 generic samplers for draw calls, can bind to any slots.. if more are required use a second set.
        // global samplers (shadow map, render targets etc, will still be bound in addition to these)
        sampler_binding sb[MAX_TECHNIQUE_SAMPLER_BINDINGS];
    };

} // namespace put

namespace put
{
    namespace pmfx
    {
        enum e_constant_widget
        {
            CW_SLIDER,
            CW_INPUT,
            CW_COLOUR,
            CW_NUM
        };

        enum e_permutation_widget
        {
            PW_CHECKBOX,
            PW_INPUT,
            PW_NUM
        };

        enum e_constant_buffer_locations
        {
            CB_PER_PASS_VIEW = 0,
            CB_PER_DRAW_CALL = 1,
            CB_FILTER_KERNEL = 2,
            CB_PER_PASS_LIGHTS = 3,
            CB_PER_PASS_SHADOW = 4,
            CB_PER_PASS_SDF_SHADOW = 5,
            CB_PER_PASS_AREA_LIGHTS = 6,
            CB_MATERIAL_CONSTANTS = 7,
            CB_SAMPLER_INFO = 10,
            CB_POST_PROCESS_INFO = 3
        };

        enum e_render_state_type : u32
        {
            RS_RASTERIZER = 0,
            RS_SAMPLER,
            RS_BLEND,
            RS_DEPTH_STENCIL
        };

        struct technique_constant
        {
            Str     name;
            hash_id id_name;
            u32     widget = CW_SLIDER;
            f32     min = 0.0f;
            f32     max = 1.0f;
            f32     step = 0.01f;
            u32     cb_offset = 0;
            u32     num_elements = 0;
        };

        struct technique_sampler
        {
            Str     name;
            hash_id id_name;
            Str     sampler_state_name;
            Str     type_name;
            Str     default_name;
            Str     filename;
            u32     handle;
            u32     unit;
            u32     sampler_state;
            u32     bind_flags;
        };

        struct technique_permutation
        {
            Str name = "";
            u32 val = 0;
            u32 widget = PW_CHECKBOX;
        };

        struct shader_program
        {
            hash_id id_name;
            hash_id id_sub_type;
            Str     name;

            u32 vertex_shader;
            u32 pixel_shader;
            u32 stream_out_shader;
            u32 compute_shader;
            u32 input_layout;
            u32 program_index;
            u32 technique_constant_size; // bytes
            u32 permutation_id;          // bitmask
            u32 permutation_option_mask; // bits which this permutation parent technique accepts

            f32*                   constant_defaults;
            technique_constant*    constants;
            technique_sampler*     textures;
            technique_permutation* permutations;
        };

        enum rt_flags
        {
            RT_AUX = 1 << 1,
            RT_AUX_USED = 1 << 2,
            RT_WRITE_ONLY = 1 << 3,
            RT_RESOLVE = 1 << 4
        };

        enum e_rt_mode
        {
            VRT_READ,
            VRT_WRITE,
            VRT_NUM
        };

        struct render_target
        {
            hash_id id_name;

            // todo replace with tcp..?
            s32 width = 0;
            s32 height = 0;
            f32 ratio = 0;
            s32 num_mips = 0;
            u32 num_arrays = 0;
            u32 format = 0;
            u32 handle = PEN_INVALID_HANDLE;
            u32 samples = 1;
            Str name = "";
            u32 flags = 0;
            u32 pp = VRT_READ;
            u32 pp_read = PEN_INVALID_HANDLE;
            u32 collection = pen::TEXTURE_COLLECTION_NONE;
        };

        struct rt_resize_params
        {
            u32       width = 0;
            u32       height = 0;
            u32       num_mips = 0;
            u32       num_arrays = 0; // num array slices
            u32       collection = pen::TEXTURE_COLLECTION_NONE;
            u32       flags = 0;
            const c8* format = nullptr;
        };

        // pmfx renderer ---------------------------------------------------------------------------------------------------

        void init(const c8* filename);
        void shutdown();
        void show_dev_ui();

        void release_script_resources();
        void render();
        void render_view(hash_id id_name);

        void register_scene(ecs::ecs_scene* scene, const char* name);
        void register_camera(camera* cam, const char* name);
        void register_scene_view_renderer(const scene_view_renderer& svr);

        void resize_render_target(hash_id target, u32 width, u32 height, const c8* format = nullptr);
        void resize_render_target(hash_id target, const rt_resize_params& params);
        void resize_viewports();

        void set_view_set(const c8* name);

        camera*              get_camera(hash_id id_name);
        camera**             get_cameras(); // call sb_free on return value when done
        const render_target* get_render_target(hash_id h);
        void                 get_render_target_dimensions(const render_target* rt, f32& w, f32& h);
        u32                  get_render_state(hash_id id_name, u32 type);
        Str                  get_render_state_name(u32 handle);
        c8**                 get_render_state_list(u32 type);    // call sb_free on return value when done
        hash_id*             get_render_state_id_list(u32 type); // call sb_free on return value when don

        void fullscreen_quad(const scene_view& sv);

        // pmfx shader -----------------------------------------------------------------------------------------------------

        u32  load_shader(const c8* pmfx_name);
        void release_shader(u32 shader);

        void set_technique(u32 shader, u32 technique_index);
        bool set_technique_perm(u32 shader, hash_id id_technique, u32 permutation = 0);

        void initialise_constant_defaults(u32 shader, u32 technique_index, f32* data);
        void initialise_sampler_defaults(u32 handle, u32 technique_index, sampler_set& samplers);

        u32                 get_technique_list_index(u32 shader, hash_id id_technique); // return index in name list
        const c8**          get_shader_list(u32& count);
        const c8**          get_technique_list(u32 shader, u32& count);
        const c8*           get_shader_name(u32 shader);
        const c8*           get_technique_name(u32 shader, hash_id id_technique);
        hash_id             get_technique_id(u32 shader, u32 technique_index);
        u32                 get_technique_index_perm(u32 shader, hash_id id_technique, u32 permutation = 0);
        technique_constant* get_technique_constants(u32 shader, u32 technique_index);
        technique_constant* get_technique_constant(hash_id id_constant, u32 shader, u32 technique_index);
        u32                 get_technique_cbuffer_size(u32 shader, u32 technique_index);
        technique_sampler*  get_technique_samplers(u32 shader, u32 technique_index);
        technique_sampler*  get_technique_sampler(hash_id id_sampler, u32 shader, u32 technique_index);

        bool show_technique_ui(u32 shader, u32 technique_index, f32* data, sampler_set& samplers, u32* permutation);
        bool has_technique_permutations(u32 shader, u32 technique_index);
        bool has_technique_constants(u32 shader, u32 technique_index);
        bool has_technique_samplers(u32 shader, u32 technique_index);
        bool has_technique_params(u32 shader, u32 technique_index);

        void poll_for_changes();
    } // namespace pmfx
} // namespace put

#endif
