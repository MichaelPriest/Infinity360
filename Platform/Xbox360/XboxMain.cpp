// Native Xbox 360 bootstrap. This translation unit is only compiled by the
// Xbox 360 XDK project; the desktop CMake target keeps using Source/main.cpp.
#include <xtl.h>
#include <d3d9.h>

namespace {
IDirect3D9* g_d3d = 0;
IDirect3DDevice9* g_device = 0;
int g_selected = 0;

bool InitializeGraphics() {
    g_d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_d3d) return false;

    D3DPRESENT_PARAMETERS parameters;
    ZeroMemory(&parameters, sizeof(parameters));
    parameters.BackBufferWidth = 1280;
    parameters.BackBufferHeight = 720;
    parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    parameters.BackBufferCount = 1;
    parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    parameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    return SUCCEEDED(g_d3d->CreateDevice(
        0, D3DDEVTYPE_HAL, 0, D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &parameters, &g_device));
}

void PollController(bool& running) {
    static WORD previous = 0;
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(state));
    if (XInputGetState(0, &state) != ERROR_SUCCESS) return;
    const WORD pressed = state.Gamepad.wButtons & ~previous;
    previous = state.Gamepad.wButtons;
    if ((pressed & XINPUT_GAMEPAD_DPAD_LEFT) && g_selected > 0) --g_selected;
    if ((pressed & XINPUT_GAMEPAD_DPAD_RIGHT) && g_selected < 4) ++g_selected;
    if (pressed & XINPUT_GAMEPAD_BACK) running = false;
}

void RenderFrame() {
    const D3DCOLOR backgrounds[] = {
        D3DCOLOR_XRGB(55, 145, 25), D3DCOLOR_XRGB(65, 43, 88),
        D3DCOLOR_XRGB(25, 70, 91), D3DCOLOR_XRGB(91, 55, 25),
        D3DCOLOR_XRGB(45, 56, 51)
    };
    g_device->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB(7, 16, 14), 1.0f, 0);
    if (SUCCEEDED(g_device->BeginScene())) {
        // Clear rectangular viewports to create the first hardware-safe tile
        // screen without requiring fonts, textures or external assets.
        for (int i = 0; i < 5; ++i) {
            D3DRECT tile = {70 + i * 235, 255, 285 + i * 235, 480};
            const D3DCOLOR color = i == g_selected
                ? D3DCOLOR_XRGB(120, 232, 25) : backgrounds[i];
            g_device->Clear(1, &tile, D3DCLEAR_TARGET, color, 1.0f, 0);
        }
        g_device->EndScene();
    }
    g_device->Present(0, 0, 0, 0);
}

void Shutdown() {
    if (g_device) { g_device->Release(); g_device = 0; }
    if (g_d3d) { g_d3d->Release(); g_d3d = 0; }
}
} // namespace

int main() {
    if (!InitializeGraphics()) return 1;
    bool running = true;
    while (running) {
        PollController(running);
        RenderFrame();
    }
    Shutdown();
    return 0;
}
