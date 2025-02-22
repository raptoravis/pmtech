// os.h
// Copyright 2014 - 2019 Alex Dixon.
// License: https://github.com/polymonster/pmtech/blob/master/license.md

#pragma once

// Tiny api with some window and os specific abstractions.

#include "pen.h"

namespace pen
{
    struct window_frame
    {
        u32 x, y, width, height;
    };

    // Window

    u32   window_init(void* params);
    void* window_get_primary_display_handle();
    void  window_get_frame(window_frame& f);
    void  window_set_frame(const window_frame& f);
    void  window_get_size(s32& width, s32& height);
    void  window_set_size(s32 width, s32 height);

    // OS
    void      os_terminate(u32 return_code);
    bool      os_update();
    void      os_set_cursor_pos(u32 client_x, u32 client_y);
    void      os_show_cursor(bool show);
    const c8* os_path_for_resource(const c8* filename);

} // namespace pen
