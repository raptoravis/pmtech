#include "../example_common.h"

using namespace put;
using namespace ecs;

pen::window_creation_params pen_window{
    1280,                    // width
    720,                     // height
    4,                       // MSAA samples
    "msaa_resolve"           // window title / process name
};

void mip_ui()
{
    bool opened = true;
    
    const pmfx::render_target* rs[] = {
        pmfx::get_render_target(PEN_HASH("msaa_colour")),
        pmfx::get_render_target(PEN_HASH("msaa_depth")),
        pmfx::get_render_target(PEN_HASH("msaa_custom"))
    };
    
    ImGui::Begin("MSAA Resolve", &opened, ImGuiWindowFlags_AlwaysAutoResize);

    u32 i = 0;
    for(auto* r : rs)
    {
        if(!r)
            continue;
        
        f32 w, h;
        pmfx::get_render_target_dimensions(r, w, h);
        ImVec2 size(w / 3, h / 3);
        ImGui::Image(IMG(r->handle), size);
        
        if(i == 0)
            ImGui::SameLine();
        
        ++i;
    }
    
    ImGui::End();
}

void example_setup(ecs::ecs_scene* scene, camera& cam)
{
    pmfx::init("data/configs/msaa_resolve.jsn");

    clear_scene(scene);

    material_resource* default_material = get_material_resource(PEN_HASH("default_material"));
    geometry_resource* capsule_resource = get_geometry_resource(PEN_HASH("capsule"));

    // add light
    u32 light = get_new_entity(scene);
    scene->names[light] = "front_light";
    scene->id_name[light] = PEN_HASH("front_light");
    scene->lights[light].colour = vec3f::one();
    scene->lights[light].direction = vec3f::one();
    scene->lights[light].type = LIGHT_TYPE_DIR;
    scene->transforms[light].translation = vec3f::zero();
    scene->transforms[light].rotation = quat();
    scene->transforms[light].scale = vec3f::one();
    scene->entities[light] |= CMP_LIGHT;
    scene->entities[light] |= CMP_TRANSFORM;

    // add some spheres
    f32   num_rows = 5;
    f32   d = 20.0f * 0.5f;
    vec3f start_pos = vec3f(-d, -d, -d);
    vec3f pos = start_pos;
    for (s32 i = 0; i < num_rows; ++i)
    {
        pos.x = start_pos.x;

        for (s32 i = 0; i < num_rows; ++i)
        {
            pos.z = start_pos.z;

            for (s32 j = 0; j < num_rows; ++j)
            {
                u32 capsule = get_new_entity(scene);
                scene->transforms[capsule].rotation = quat();
                scene->transforms[capsule].scale = vec3f(2.0f, 2.0f, 2.0f);
                scene->transforms[capsule].translation = pos;
                scene->parents[capsule] = capsule;
                scene->entities[capsule] |= CMP_TRANSFORM;

                instantiate_geometry(capsule_resource, scene, capsule);
                instantiate_material(default_material, scene, capsule);
                instantiate_model_cbuffer(scene, capsule);

                pos.z += d / 2;
            }

            pos.x += d / 2;
        }

        pos.y += d / 2;
    }
}

void example_update(ecs::ecs_scene* scene, camera& cam, f32 dt)
{
    mip_ui();
}
