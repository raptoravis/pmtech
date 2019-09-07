// os.cpp
// Copyright 2014 - 2019 Alex Dixon.
// License: https://github.com/polymonster/pmtech/blob/master/license.md

#include <windows.h>

#include "input.h"
#include "os.h"
#include "pen.h"
#include "pen_string.h"
#include "renderer.h"
#include "threads.h"
#include "timer.h"

#include "data_struct.h"
#include "str_utilities.h"

#include "optick.h"

extern a_u8    g_window_resize;
pen::user_info pen_user_info;

namespace pen
{
    void renderer_init(void*, bool);
}

struct window_params
{
    HINSTANCE hinstance;
    int       cmdshow;
};

static u32 s_return_code = 0;

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
{
	//OPTICK_APP("pmtechApp");

    // console
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
    {
        AllocConsole();
    }

    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

    Str str_cmd = lpCmdLine;
    if (pen::str_find(str_cmd, "-test") != -1)
    {
        pen::renderer_test_enable();
    }

    // get working directory name
    char module_filename[MAX_PATH];
    GetModuleFileNameA(hInstance, module_filename, MAX_PATH);

    static Str working_directory = module_filename;
    working_directory = pen::str_normalise_filepath(working_directory);

    // remove exe
    u32 dir = pen::str_find_reverse(working_directory, "/");
    working_directory = pen::str_substr(working_directory, 0, dir + 1);

    pen_user_info.working_directory = working_directory.c_str();

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    window_params wp;
    wp.cmdshow = nCmdShow;
    wp.hinstance = hInstance;

    if (pen::window_init(((void*)&wp)))
        return 0;

    // initilaise any generic systems
    pen::timer_system_intialise();
    pen::input_gamepad_init();

    // init renderer will enter a loop wait for rendering commands, and call os update
    HWND hwnd = (HWND)pen::window_get_primary_display_handle();
    pen::renderer_init((void*)&hwnd, true);

	OPTICK_SHUTDOWN();

    // exit program
    return s_return_code;
}

extern pen::window_creation_params pen_window;

namespace
{
    enum os_cmd_id
    {
        OS_CMD_NULL = 0,
        OS_CMD_SET_WINDOW_FRAME
    };

    struct os_cmd
    {
        u32 cmd_index;

        union {
            struct
            {
                pen::window_frame frame;
            };
        };
    };

    pen::ring_buffer<os_cmd> s_cmd_buffer;
} // namespace

namespace pen
{
    LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    HWND             g_hwnd = nullptr;
    HINSTANCE        g_hinstance = nullptr;
    static bool      terminate_app = false;

    bool os_update()
    {
        static bool init_jobs = true;
        if (init_jobs)
        {
            pen::default_thread_info thread_info;
            thread_info.flags = pen::PEN_CREATE_AUDIO_THREAD;
            pen::jobs_create_default(thread_info);
            init_jobs = false;
        }

        MSG msg = {0};

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        os_cmd* cmd = s_cmd_buffer.get();
        while (cmd)
        {
            // process cmd
            switch (cmd->cmd_index)
            {
                case OS_CMD_SET_WINDOW_FRAME:
                {
                    SetWindowPos(g_hwnd, HWND_TOP, cmd->frame.x, cmd->frame.y, cmd->frame.width, cmd->frame.height, 0);

                    RECT r;
                    GetClientRect(g_hwnd, &r);

                    pen_window.width = r.right - r.left;
                    pen_window.height = r.bottom - r.top;

                    g_window_resize = 1;
                }
                break;
                default:
                    break;
            }

            // get next
            cmd = s_cmd_buffer.get();
        }

        pen::input_gamepad_update();

        if (WM_QUIT == msg.message)
            terminate_app = true;

        if (terminate_app)
        {
            if (pen::jobs_terminate_all())
                return false;
        }

        // continue updating
        return true;
    }

    struct window_params
    {
        HINSTANCE hinstance;
        int       cmdshow;
    };

    void os_terminate(u32 return_code)
    {
        s_return_code = return_code;
        terminate_app = true;
    }

    u32 window_init(void* params)
    {
        s_cmd_buffer.create(32);

        window_params* wp = (window_params*)params;

        // Register class
        WNDCLASSEXA wcex;
        ZeroMemory(&wcex, sizeof(WNDCLASSEXA));
        wcex.cbSize = sizeof(WNDCLASSEXA);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = wp->hinstance;
        wcex.hIcon = NULL;
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = pen_window.window_title;
        wcex.hIconSm = NULL;

        if (!RegisterClassExA(&wcex))
            return E_FAIL;

        // Create window
        g_hinstance = wp->hinstance;

        // pass in as params
        RECT rc = {0, 0, (LONG)pen_window.width, (LONG)pen_window.height};
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        RECT desktop_rect;
        GetClientRect(GetDesktopWindow(), &desktop_rect);

        LONG screen_mid_x = (desktop_rect.right - desktop_rect.left) / 2;
        LONG screen_mid_y = (desktop_rect.bottom - desktop_rect.top) / 2;

        LONG half_window_x = (rc.right - rc.left) / 2;
        LONG half_window_y = (rc.bottom - rc.top) / 2;

        g_hwnd = CreateWindowA(pen_window.window_title, pen_window.window_title, WS_OVERLAPPEDWINDOW,
                               screen_mid_x - half_window_x, screen_mid_y - half_window_y, rc.right - rc.left,
                               rc.bottom - rc.top, nullptr, nullptr, wp->hinstance, nullptr);

        DWORD lasterror = GetLastError();

        if (!g_hwnd)
            return E_FAIL;

        ShowWindow(g_hwnd, wp->cmdshow);

        SetForegroundWindow(g_hwnd);

        return S_OK;
    }

    void set_unicode_key(u32 key_index, bool down)
    {
        wchar_t buff[10];

        BYTE keyState[256] = {0};

        int result =
            ToUnicodeEx(key_index, MapVirtualKey(key_index, MAPVK_VK_TO_VSC), keyState, buff, _countof(buff), 0, NULL);

        u32 unicode = buff[0];

        if (unicode > 511)
            return;

        if (down)
            pen::input_set_unicode_key_down(unicode);
        else
            pen::input_set_unicode_key_up(unicode);
    }

    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        PAINTSTRUCT ps;
        HDC         hdc;

        switch (message)
        {
            case WM_PAINT:
                hdc = BeginPaint(hWnd, &ps);
                EndPaint(hWnd, &ps);
                break;

            case WM_DESTROY:
                PostQuitMessage(0);
                break;

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                pen::input_set_key_down((u32)wParam);
                set_unicode_key((u32)wParam, true);
                break;

            case WM_KEYUP:
            case WM_SYSKEYUP:
                pen::input_set_key_up((u32)wParam);
                set_unicode_key((u32)wParam, false);
                break;

            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
                pen::input_set_mouse_down((message - 0x201) / 3);
                break;

            case WM_LBUTTONUP:
            case WM_MBUTTONUP:
            case WM_RBUTTONUP:
                pen::input_set_mouse_up((message - 0x202) / 3);
                break;

            case WM_MOUSEMOVE:
                pen::input_set_mouse_pos((f32)LOWORD(lParam), (f32)HIWORD(lParam));
                break;

            case WM_MOUSEWHEEL:
            {
                s16 low_w = (s16)LOWORD(wParam);
                s16 hi_w = (s16)HIWORD(wParam);

                s16 low_l = (s16)LOWORD(lParam);
                s16 hi_l = (s16)HIWORD(lParam);

                pen::input_set_mouse_wheel((f32)(hi_w / WHEEL_DELTA) * 10.0f);
            }
            break;

            case WM_SIZE:
            {
                s16 lo = LOWORD(wParam);

                if (lo == SIZE_MINIMIZED)
                    break;

                if (g_window_resize == 0)
                {
                    pen_window.width = LOWORD(lParam);
                    pen_window.height = HIWORD(lParam);

                    g_window_resize = 1;
                }
            }
            break;

            default:
                return DefWindowProcA(hWnd, message, wParam, lParam);
        }

        return 0;
    }

    void* window_get_primary_display_handle()
    {
        return g_hwnd;
    }

    void window_get_frame(window_frame& f)
    {
        RECT r;
        GetWindowRect(g_hwnd, &r);

        f.x = r.left;
        f.y = r.top;

        f.width = r.right - r.left;
        f.height = r.bottom - r.top;
    }

    void window_set_frame(const window_frame& f)
    {
        os_cmd cmd;
        cmd.cmd_index = OS_CMD_SET_WINDOW_FRAME;
        cmd.frame = f;

        s_cmd_buffer.put(cmd);
    }

    void os_set_cursor_pos(u32 client_x, u32 client_y)
    {
        HWND  hw = (HWND)pen::window_get_primary_display_handle();
        POINT p = {(LONG)client_x, (LONG)client_y};

        ClientToScreen(hw, &p);
        SetCursorPos(p.x, p.y);
    }

    void os_show_cursor(bool show)
    {
        ShowCursor(show);
    }

    bool input_undo_pressed()
    {
        return pen::input_key(PK_CONTROL) && pen::input_key(PK_Z);
    }

    bool input_redo_pressed()
    {
        return pen::input_key(PK_CONTROL) && pen::input_key(PK_Y);
    }

    const c8* os_path_for_resource(const c8* filename)
    {
        return filename;
    }
} // namespace pen
