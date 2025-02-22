// input.cpp
// Copyright 2014 - 2019 Alex Dixon.
// License: https://github.com/polymonster/pmtech/blob/master/license.md

#include "input.h"
#include "console.h"
#include "data_struct.h"
#include "os.h"
#include "threads.h"
#include "timer.h"
#include <atomic>
#include <math.h>

// Keyboard and mouse
#define KEY_PRESS 0x01
#define KEY_HELD 0x02

namespace pen
{
    u8 keyboard_state[PK_ARRAY_SIZE];
    u8 ascii_state[PK_ARRAY_SIZE];

    mouse_state mouse_state_ = {0};

    std::atomic<bool> show_cursor = {true};

    void input_set_unicode_key_down(u32 key_index)
    {
        ascii_state[key_index] = 1;
    }

    void input_set_unicode_key_up(u32 key_index)
    {
        ascii_state[key_index] = 0;
    }

    bool input_get_unicode_key(u32 key_index)
    {
        return ascii_state[key_index] == 1;
    }

    void input_set_key_down(u32 key_index)
    {
        keyboard_state[key_index]++;
    }

    void input_set_key_up(u32 key_index)
    {
        keyboard_state[key_index] = 0;
    }

    bool input_is_key_pressed(u32 key_index)
    {
        return keyboard_state[key_index] == KEY_PRESS;
    }

    bool input_is_key_held(u32 key_index)
    {
        return keyboard_state[key_index] >= KEY_HELD;
    }

    void input_set_mouse_down(u32 button_index)
    {
        mouse_state_.buttons[button_index]++;
    }

    void input_set_mouse_up(u32 button_index)
    {
        mouse_state_.buttons[button_index] = 0;
    }

    const pen::mouse_state& input_get_mouse_state()
    {
        return mouse_state_;
    }

    bool input_is_mouse_pressed(u32 button_index)
    {
        return mouse_state_.buttons[button_index] == KEY_PRESS;
    }

    bool input_is_mouse_held(u32 button_index)
    {
        return mouse_state_.buttons[button_index] == KEY_PRESS;
    }

    void input_set_mouse_pos(f32 x, f32 y)
    {
        mouse_state_.x = x;
        mouse_state_.y = y;
    }

    void input_set_mouse_wheel(f32 wheel)
    {
        mouse_state_.wheel += wheel;
    }

    bool input_is_key_down(u32 key_index)
    {
        return (input_is_key_held(key_index) || input_is_key_pressed(key_index));
    }

    bool input_is_mouse_down(u32 button_index)
    {
        return (input_is_mouse_held(button_index) || input_is_mouse_pressed(button_index));
    }

    void input_set_cursor_pos(u32 client_x, u32 client_y)
    {
        os_set_cursor_pos(client_x, client_y);
    }

    void input_show_cursor(bool show)
    {
        show_cursor = show;
    }

    const c8* input_get_key_str(u32 key_index)
    {
        switch (key_index)
        {
            case PK_LBUTTON:
                return "L button";
            case PK_RBUTTON:
                return "R button";
            case PK_CANCEL:
                return "cancel";
            case PK_MBUTTON:
                return "M button";
            case PK_BACK:
                return "back";
            case PK_TAB:
                return "tab";
            case PK_CLEAR:
                return "clear";
            case PK_RETURN:
                return "return";
            case PK_SHIFT:
                return "shift";
            case PK_CONTROL:
                return "ctrl";
            case PK_MENU:
                return "menu";
            case PK_PAUSE:
                return "pause";
            case PK_CAPITAL:
                return "caps lock";
            case PK_ESCAPE:
                return "escape";
            case PK_SPACE:
                return "space";
            case PK_PRIOR:
                return "page down";
            case PK_NEXT:
                return "page up";
            case PK_HOME:
                return "home";
            case PK_LEFT:
                return "left";
            case PK_UP:
                return "up";
            case PK_RIGHT:
                return "right";
            case PK_DOWN:
                return "down";
            case PK_SELECT:
                return "select";
            case PK_EXECUTE:
                return "execute";
            case PK_INSERT:
                return "insert";
            case PK_SNAPSHOT:
                return "screen shot";
            case PK_DELETE:
                return "delete";
            case PK_HELP:
                return "help";
            case PK_F1:
                return "f1";
            case PK_F2:
                return "f2";
            case PK_F3:
                return "f3";
            case PK_F4:
                return "f4";
            case PK_F5:
                return "f5";
            case PK_F6:
                return "f6";
            case PK_F7:
                return "f7";
            case PK_F8:
                return "f8";
            case PK_F9:
                return "f9";
            case PK_F10:
                return "f10";
            case PK_F11:
                return "f11";
            case PK_F12:
                return "f12";
            case PK_NUMLOCK:
                return "num lock";
            case PK_SCROLL:
                return "scroll";
            case PK_COMMAND:
                return "command";
        }

        static c8 key_char[512] = {0};
        if (key_char[2] == 0)
        {
            for (s32 i = 0; i < 256; ++i)
            {
                key_char[i * 2] = (c8)i;
            }
        }

        if (key_index < 256)
        {
            return &key_char[key_index * 2];
        }

        return "unknown";
    }
} // namespace pen

// Gamepad ------------------------------------------------------------------------------------------------------------------

#define API_RAW_INPUT 0
#define API_XINPUT 1
#define TRIGGER_X360 1024
#define DPAD_X_AXIS 1025
#define DPAD_Y_AXIS 1026

#if __APPLE__
#include "TargetConditionals.h"
#endif

extern "C"
{
#include "gamepad/Gamepad.h"
#include "gamepad/Gamepad_private.c"
#ifdef __linux__
#define _API API_RAW_INPUT
#include "gamepad/Gamepad_linux.c"
#elif _WIN32
#define _API API_XINPUT
#include "gamepad/Gamepad_windows_dinput.c"
#else //macos, ios
#define _API API_RAW_INPUT
#if TARGET_OS_IPHONE
// make stub if required
#else
#include "gamepad/Gamepad_macosx.c"
#endif
#endif
}

namespace
{
    struct device_mapping
    {
        u32 vendor_id;
        u32 product_id;
        u32 api_id;
        u32 button_map[PGP_MAX_BUTTONS];
        u32 axis_map[PGP_MAX_AXIS];
        f32 axis_flip[PGP_MAX_AXIS];
    };

    pen::raw_gamepad_state s_raw_gamepads[PGP_MAX_GAMEPADS] = {};
    pen::gamepad_state     s_gamepads[PGP_MAX_GAMEPADS] = {};
    device_mapping*        s_device_maps = nullptr;

    void init_map(device_mapping& map)
    {
        for (u32 b = 0; b < PGP_MAX_BUTTONS; ++b)
            map.button_map[b] = PEN_INVALID_HANDLE;

        for (u32 a = 0; a < PGP_MAX_AXIS; ++a)
            map.axis_map[a] = PEN_INVALID_HANDLE;

        for (u32 a = 0; a < PGP_MAX_AXIS; ++a)
            map.axis_flip[a] = 1.0f;
    }

    void init_gamepad_values(pen::gamepad_state& gs)
    {
        for (u32 b = 0; b < PGP_BUTTON_NUM; ++b)
            gs.button[b] = 0;

        for (u32 a = 0; a < PGP_AXIS_NUM; ++a)
            gs.axis[a] = 0.0f;

        gs.axis[PGP_AXIS_LTRIGGER] = -1.0f;
        gs.axis[PGP_AXIS_RTRIGGER] = -1.0f;
    }

    void init_gamepad_mappings()
    {
        // No Mapping
        device_mapping no;
        no.vendor_id = 0;
        no.product_id = 0;
        init_map(no);
        for (u32 i = 0; i < PGP_BUTTON_NUM; ++i)
            no.button_map[i] = i;

        for (u32 i = 0; i < PGP_AXIS_NUM; ++i)
            no.axis_map[i] = i;

        sb_push(s_device_maps, no);

        // DS4
        device_mapping ps4;
        init_map(ps4);
        ps4.vendor_id = 1356;
        ps4.product_id = 1476;
        ps4.api_id = API_RAW_INPUT;
        ps4.button_map[0] = PGP_BUTTON_X; // SQUARE
        ps4.button_map[1] = PGP_BUTTON_A; // X
        ps4.button_map[2] = PGP_BUTTON_B; // CIRCLE
        ps4.button_map[3] = PGP_BUTTON_Y; // TRIANGLE
        ps4.button_map[4] = PGP_BUTTON_L1;
        ps4.button_map[5] = PGP_BUTTON_R1;
        ps4.button_map[8] = PGP_BUTTON_BACK;
        ps4.button_map[9] = PGP_BUTTON_START;
        ps4.button_map[10] = PGP_BUTTON_L3;
        ps4.button_map[11] = PGP_BUTTON_R3;
        ps4.button_map[12] = PGP_BUTTON_PLATFORM;
        ps4.button_map[13] = PGP_BUTTON_TOUCH_PAD;
        ps4.axis_map[0] = PGP_AXIS_LEFT_STICK_X;
        ps4.axis_map[1] = PGP_AXIS_LEFT_STICK_Y;
        ps4.axis_map[2] = PGP_AXIS_RIGHT_STICK_X;
        ps4.axis_map[3] = PGP_AXIS_RIGHT_STICK_Y;
        ps4.axis_map[4] = DPAD_X_AXIS;
        ps4.axis_map[5] = DPAD_Y_AXIS;
        ps4.axis_map[6] = PGP_AXIS_LTRIGGER;
        ps4.axis_map[7] = PGP_AXIS_RTRIGGER;
        sb_push(s_device_maps, ps4);

        // Xbox 360
        device_mapping x360;
        init_map(x360);
        x360.vendor_id = 1118;
        x360.product_id = 654;
        x360.api_id = API_RAW_INPUT;
        x360.button_map[0] = PGP_BUTTON_A;
        x360.button_map[1] = PGP_BUTTON_B;
        x360.button_map[2] = PGP_BUTTON_X;
        x360.button_map[3] = PGP_BUTTON_Y;
        x360.button_map[4] = PGP_BUTTON_L1;
        x360.button_map[5] = PGP_BUTTON_R1;
        x360.button_map[6] = PGP_BUTTON_BACK;
        x360.button_map[7] = PGP_BUTTON_START;
        x360.button_map[8] = PGP_BUTTON_L3;
        x360.button_map[9] = PGP_BUTTON_R3;
        x360.axis_map[0] = PGP_AXIS_LEFT_STICK_X;
        x360.axis_map[1] = PGP_AXIS_LEFT_STICK_Y;
        x360.axis_map[2] = TRIGGER_X360;
        x360.axis_map[3] = PGP_AXIS_RIGHT_STICK_X;
        x360.axis_map[4] = PGP_AXIS_RIGHT_STICK_Y;
        x360.axis_map[5] = DPAD_X_AXIS;
        x360.axis_map[6] = DPAD_Y_AXIS;
        sb_push(s_device_maps, x360);

        // DS4 with windows XInput
        device_mapping ps4x;
        init_map(ps4x);
        ps4x = ps4;
        ps4x.vendor_id = 1356;
        ps4x.product_id = 1476;
        ps4x.api_id = API_XINPUT;
        ps4x.axis_map[0] = PGP_AXIS_RIGHT_STICK_Y;
        ps4x.axis_map[1] = PGP_AXIS_RIGHT_STICK_X;
        ps4x.axis_map[2] = PGP_AXIS_LEFT_STICK_Y;
        ps4x.axis_map[3] = PGP_AXIS_LEFT_STICK_X;
        ps4x.axis_map[7] = PGP_AXIS_LTRIGGER;
        ps4x.axis_map[6] = PGP_AXIS_RTRIGGER;
        sb_push(s_device_maps, ps4x);

        // Xbox 360 with XInput
        device_mapping x360x;
        init_map(x360x);
        x360x.vendor_id = 1118;
        x360x.product_id = 654;
        x360x.api_id = API_XINPUT;
        x360x.button_map[0] = PGP_BUTTON_DUP;
        x360x.button_map[1] = PGP_BUTTON_DDOWN;
        x360x.button_map[2] = PGP_BUTTON_DLEFT;
        x360x.button_map[3] = PGP_BUTTON_DRIGHT;
        x360x.button_map[10] = PGP_BUTTON_A;
        x360x.button_map[11] = PGP_BUTTON_B;
        x360x.button_map[12] = PGP_BUTTON_X;
        x360x.button_map[13] = PGP_BUTTON_Y;
        x360x.button_map[8] = PGP_BUTTON_L1;
        x360x.button_map[9] = PGP_BUTTON_R1;
        x360x.button_map[5] = PGP_BUTTON_BACK;
        x360x.button_map[4] = PGP_BUTTON_START;
        x360x.button_map[6] = PGP_BUTTON_L3;
        x360x.button_map[7] = PGP_BUTTON_R3;
        x360x.axis_map[0] = PGP_AXIS_LEFT_STICK_X;
        x360x.axis_map[1] = PGP_AXIS_LEFT_STICK_Y;
        x360x.axis_map[2] = PGP_AXIS_RIGHT_STICK_X;
        x360x.axis_map[3] = PGP_AXIS_RIGHT_STICK_Y;
        x360x.axis_map[4] = PGP_AXIS_LTRIGGER;
        x360x.axis_map[5] = PGP_AXIS_RTRIGGER;
        x360x.axis_flip[1] = -1.0f;
        x360x.axis_flip[3] = -1.0f;
        sb_push(s_device_maps, x360x);
    }

    void map_button(u32 gamepad, u32 button)
    {
        u32 gi = gamepad;
        u32 mapping = s_raw_gamepads[gi].mapping;
        if (mapping == PEN_INVALID_HANDLE)
            return;

        u32 rb = s_device_maps[mapping].button_map[button];
        if (rb == PEN_INVALID_HANDLE)
            return;

        s_gamepads[gi].button[rb] = s_raw_gamepads[gi].button[button];
    }

    void map_axis(u32 gamepad, u32 axis)
    {
        u32 gi = gamepad;
        u32 mapping = s_raw_gamepads[gi].mapping;
        if (mapping == PEN_INVALID_HANDLE)
            return;

        u32 ra = s_device_maps[mapping].axis_map[axis];
        if (ra == PEN_INVALID_HANDLE)
            return;

        // apply basic mapping and bail early
        if (ra < TRIGGER_X360)
        {
            s_gamepads[gi].axis[ra] = s_raw_gamepads[gi].axis[axis] * s_device_maps[mapping].axis_flip[axis];
            return;
        }

        // special mapping
        switch (ra)
        {
            case TRIGGER_X360:
            {
                // direct input only supplies 1 axis for both triggers
                f32 raw = s_raw_gamepads[gi].axis[axis];
                if (raw < 0.0f)
                {
                    s_gamepads[gi].axis[PGP_AXIS_LTRIGGER] = fabs(raw) * 2.0f - 1.0f;
                }
                else
                {
                    s_gamepads[gi].axis[PGP_AXIS_RTRIGGER] = fabs(raw) * 2.0f - 1.0f;
                }
            }
            break;

            case DPAD_X_AXIS:
            {
                // ps4 supplies the dpad as axis
                f32 raw = s_raw_gamepads[gi].axis[axis];
                s_gamepads[gi].button[PGP_BUTTON_DLEFT] = 0;
                s_gamepads[gi].button[PGP_BUTTON_DRIGHT] = 0;

                if (raw < 0.0f)
                {
                    s_gamepads[gi].button[PGP_BUTTON_DLEFT] = 1;
                }
                else if (raw > 0.0f)
                {
                    s_gamepads[gi].button[PGP_BUTTON_DRIGHT] = 1;
                }
            }
            break;

            case DPAD_Y_AXIS:
            {
                // ps4 supplies the dpad as axis
                f32 raw = s_raw_gamepads[gi].axis[axis];
                s_gamepads[gi].button[PGP_BUTTON_DUP] = 0;
                s_gamepads[gi].button[PGP_BUTTON_DDOWN] = 0;

                if (raw < 0.0f)
                {
                    s_gamepads[gi].button[PGP_BUTTON_DUP] = 1;
                }
                else if (raw > 0.0f)
                {
                    s_gamepads[gi].button[PGP_BUTTON_DDOWN] = 1;
                }
            }
            break;
        }
    }

    void update_gamepad(Gamepad_device* device, u32 axis, u32 button)
    {
        u32 gi = device->deviceID;

        if (gi >= PGP_MAX_GAMEPADS)
            return;

        s_raw_gamepads[gi].device_id = device->deviceID;
        s_raw_gamepads[gi].vendor_id = device->vendorID;
        s_raw_gamepads[gi].product_id = device->productID;

        if (button < PGP_MAX_BUTTONS)
            s_raw_gamepads[gi].button[button] = device->buttonStates[button];

        if (axis <= PGP_MAX_AXIS)
            s_raw_gamepads[gi].axis[axis] = device->axisStates[axis];
    }

    void map_gamepad(u32 gamepad)
    {
        u32 gi = gamepad;

        if (s_raw_gamepads[gi].vendor_id == PEN_INVALID_HANDLE)
            return;

        if (s_raw_gamepads[gi].mapping != PEN_INVALID_HANDLE)
            return;

        s_raw_gamepads[gi].mapping = 0;

        u32 num_maps = sb_count(s_device_maps);
        for (u32 i = 0; i < num_maps; ++i)
        {
            if (s_raw_gamepads[gi].vendor_id != s_device_maps[i].vendor_id)
                continue;

            if (s_raw_gamepads[gi].product_id != s_device_maps[i].product_id)
                continue;

            if (_API != s_device_maps[i].api_id)
                continue;

            s_raw_gamepads[gi].mapping = i;
            break;
        }
    }
} // namespace
namespace pen
{
    void gamepad_attach_func(struct Gamepad_device* device, void* context)
    {
        update_gamepad(device, -1, -1);

        // find mapping for buttons and axes
        u32 gi = device->deviceID;

        // init vals
        init_gamepad_values(s_gamepads[gi]);
        map_gamepad(gi);
    }

    void gamepad_remove_func(struct Gamepad_device* device, void* context)
    {
    }

    void gamepad_button_down_func(struct Gamepad_device* device, u32 button_id, f64 timestamp, void* context)
    {
        update_gamepad(device, -1, button_id);
        map_button(device->deviceID, button_id);
    }

    void gamepad_button_up_func(struct Gamepad_device* device, u32 button_id, f64 timestamp, void* context)
    {
        update_gamepad(device, -1, button_id);
        map_button(device->deviceID, button_id);
    }

    void gamepad_axis_move_func(struct Gamepad_device* device, u32 axis_id, f32 value, f32 last_value, f64 timestamp,
                                void* context)
    {
        update_gamepad(device, axis_id, -1);
        map_axis(device->deviceID, axis_id);
    }

    void input_gamepad_shutdown()
    {
        Gamepad_shutdown();
    }

    std::atomic<bool> a_detecting;
    PEN_TRV           detect_devices_async(void* params)
    {
        a_detecting = true;

        pen::job_thread_params* job_params = (pen::job_thread_params*)params;

        pen::job* p_thread_info = job_params->job_info;
        pen::semaphore_post(p_thread_info->p_sem_continue, 1);

        Gamepad_detectDevices();

        pen::semaphore_post(p_thread_info->p_sem_continue, 1);
        pen::semaphore_post(p_thread_info->p_sem_terminated, 1);

        a_detecting = false;
        return PEN_THREAD_OK;
    }

    void input_gamepad_update()
    {
        Gamepad_processEvents();

        // detect devices
        static pen::timer* htimer = timer_create();
        static const f32 detect_time = 10000.0f;
        static f32       detect_timer = detect_time;
        if (detect_timer <= 0)
        {
            pen::jobs_create_job(detect_devices_async, 1024 * 1024, nullptr, pen::THREAD_START_DETACHED);
            detect_timer = detect_time;
        }

        // check device mappings
        u32 num_gp = input_get_num_gamepads();
        for (u32 i = 0; i < num_gp; ++i)
        {
            map_gamepad(i);
        }

        detect_timer -= timer_elapsed_ms(htimer);
        timer_start(htimer);
    }

    u32 input_get_num_gamepads()
    {
        return Gamepad_numDevices();
    }

    void input_gamepad_init()
    {
        Gamepad_init();

        // register call backs
        Gamepad_deviceAttachFunc(gamepad_attach_func, nullptr);
        Gamepad_deviceRemoveFunc(gamepad_remove_func, nullptr);
        Gamepad_buttonDownFunc(gamepad_button_down_func, nullptr);
        Gamepad_buttonUpFunc(gamepad_button_up_func, nullptr);
        Gamepad_axisMoveFunc(gamepad_axis_move_func, nullptr);

        init_gamepad_mappings();

        Gamepad_detectDevices();

        a_detecting = false;
    }

    void input_get_gamepad_state(u32 device_index, gamepad_state& gs)
    {
        gs = s_gamepads[device_index];
    }

    void input_get_raw_gamepad_state(u32 device_index, raw_gamepad_state& gs)
    {
        gs = s_raw_gamepads[device_index];
    }
} // namespace pen
