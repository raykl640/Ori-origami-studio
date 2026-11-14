// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Studio : the window shell (SDL2)             Owner : Ray Lukorito (App shell)
//
//    ori_studio [FILE.ori | FILE.cp] [--scale N] [--size WxH]
//    ori_studio --screenshot out.png [--size WxH] [FILE]   render one frame, no window
//
//  SDL2 is used ONLY to open a window, read the mouse / keyboard and show
//  the finished frame. Every pixel of the studio is drawn by ORI's own
//  rasterizer into an ori::Image (see studio/Studio.cpp); this file copies
//  that framebuffer into a streaming texture once per frame.
// ============================================================================
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "../Studio.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace ori;

namespace {

KeyPress mapKey(const SDL_Keysym& ks) {
    KeyPress k;
    const Uint16 mod = ks.mod;
    k.ctrl = (mod & (KMOD_CTRL | KMOD_GUI)) != 0;
    k.shift = (mod & KMOD_SHIFT) != 0;
    k.alt = (mod & KMOD_ALT) != 0;
    switch (ks.sym) {
        case SDLK_ESCAPE: k.key = Key::Escape; break;
        case SDLK_RETURN: case SDLK_KP_ENTER: k.key = Key::Enter; break;
        case SDLK_BACKSPACE: k.key = Key::Backspace; break;
        case SDLK_DELETE: k.key = Key::Delete; break;
        case SDLK_LEFT: k.key = Key::Left; break;
        case SDLK_RIGHT: k.key = Key::Right; break;
        case SDLK_UP: k.key = Key::Up; break;
        case SDLK_DOWN: k.key = Key::Down; break;
        case SDLK_HOME: k.key = Key::Home; break;
        case SDLK_END: k.key = Key::End; break;
        case SDLK_PAGEUP: k.key = Key::PageUp; break;
        case SDLK_PAGEDOWN: k.key = Key::PageDown; break;
        case SDLK_SPACE: k.key = Key::Space; break;
        case SDLK_TAB: k.key = Key::Tab; break;
        case SDLK_F1: k.key = Key::F1; break;
        case SDLK_F2: k.key = Key::F2; break;
        case SDLK_KP_PLUS: k.key = Key::Char; k.ch = '+'; break;
        case SDLK_KP_MINUS: k.key = Key::Char; k.ch = '-'; break;
        default:
            if (ks.sym >= 32 && ks.sym < 127) {
                k.key = Key::Char;
                k.ch = char(ks.sym);
                if (k.shift && k.ch == '/') k.ch = '?';
                if (k.shift && k.ch == '=') k.ch = '+';
            }
            break;
    }
    return k;
}

bool parseSize(const char* s, int& w, int& h) { return std::sscanf(s, "%dx%d", &w, &h) == 2 && w >= 640 && h >= 480; }

void usage() {
    std::printf("ori_studio [FILE.ori | FILE.cp] [--scale N] [--size WxH]\n"
                "ori_studio --screenshot OUT.png [--size WxH] [FILE]\n");
}

} // namespace

int main(int argc, char** argv) {
    std::string file, screenshot;
    int scale = 0, width = 1280, height = 800;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--scale" && i + 1 < argc) scale = std::clamp(std::atoi(argv[++i]), 1, 4);
        else if (a == "--size" && i + 1 < argc) { if (!parseSize(argv[++i], width, height)) { usage(); return 2; } }
        else if (a == "--screenshot" && i + 1 < argc) screenshot = argv[++i];
        else if (a == "-h" || a == "--help") { usage(); return 0; }
        else if (!a.empty() && a[0] != '-') file = a;
        else { usage(); return 2; }
    }

    Studio studio;
    if (!file.empty()) studio.open(file);

    // Headless: render one frame to a file (used for docs and CI, needs no display).
    if (!screenshot.empty()) {
        Image frame(width, height);
        UiInput in;
        studio.frame(frame, in, 0.0);
        studio.frame(frame, in, 0.0);
        if (!frame.save(screenshot)) { std::fprintf(stderr, "cannot write %s\n", screenshot.c_str()); return 1; }
        std::printf("%s\n", screenshot.c_str());
        return 0;
    }

    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");   // nearest-neighbour: crisp pixel font when scaled
    SDL_Window* window = SDL_CreateWindow("ORI Studio", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowMinimumSize(window, 1100, 700);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // UI scale: 1 framebuffer pixel = `scale` screen pixels. Default: 2 on very
    // large (HiDPI) outputs so the 5x7 font stays readable.
    int outW = 0, outH = 0;
    SDL_GetRendererOutputSize(renderer, &outW, &outH);
    if (scale == 0) scale = outW >= 2400 ? 2 : 1;

    SDL_Texture* texture = nullptr;
    Image frame;
    UiInput input;
    bool redraw = true;
    Uint64 last = SDL_GetPerformanceCounter();
    SDL_StartTextInput();   // text events only matter while a dialog field is shown

    while (!studio.shouldExit()) {
        SDL_Event e;
        const bool animate = studio.needsAnimation();
        // Block until something happens, unless a frame is already owed.
        bool got = redraw ? SDL_PollEvent(&e) != 0
                 : SDL_WaitEventTimeout(&e, animate ? 15 : 250) != 0;
        while (got) {
            int winW = 1, winH = 1;
            SDL_GetWindowSize(window, &winW, &winH);
            SDL_GetRendererOutputSize(renderer, &outW, &outH);
            // window coordinates -> framebuffer pixels
            const double fx = double(outW) / std::max(1, winW) / scale, fy = double(outH) / std::max(1, winH) / scale;
            switch (e.type) {
                case SDL_QUIT: studio.requestQuit(); break;
                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_CLOSE) studio.requestQuit();
                    break;
                case SDL_MOUSEMOTION:
                    input.mouseX = int(e.motion.x * fx);
                    input.mouseY = int(e.motion.y * fy);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    input.mouseX = int(e.button.x * fx);
                    input.mouseY = int(e.button.y * fy);
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        input.mouseDown = true;
                        input.pressed = true;
                        input.doubleClick = e.button.clicks >= 2;
                    } else if (e.button.button == SDL_BUTTON_RIGHT) {
                        input.rightPressed = true;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        input.mouseDown = false;
                        input.released = true;
                    }
                    break;
                case SDL_MOUSEWHEEL:
                    input.wheel += e.wheel.y;
                    break;
                case SDL_KEYDOWN: {
                    const KeyPress k = mapKey(e.key.keysym);
                    if (k.ctrl && (k.is('=') || k.is('+'))) { scale = std::min(4, scale + 1); break; }
                    if (k.ctrl && k.is('-')) { scale = std::max(1, scale - 1); break; }
                    if (k.key != Key::None) input.keys.push_back(k);
                    break;
                }
                case SDL_TEXTINPUT:
                    input.text += e.text.text;
                    break;
                case SDL_DROPFILE:
                    input.droppedFiles.push_back(e.drop.file);
                    SDL_free(e.drop.file);
                    break;
                default: break;
            }
            redraw = true;
            got = SDL_PollEvent(&e) != 0;
        }
        const SDL_Keymod mod = SDL_GetModState();
        input.shift = (mod & KMOD_SHIFT) != 0;
        input.ctrl = (mod & (KMOD_CTRL | KMOD_GUI)) != 0;
        input.alt = (mod & KMOD_ALT) != 0;
        // A press and release inside one batch: keep "pressed" this frame and
        // report the release next frame, so clicks are never lost.
        bool pendingRelease = false;
        if (input.pressed && input.released) { input.released = false; input.mouseDown = true; pendingRelease = true; }

        const Uint64 now = SDL_GetPerformanceCounter();
        const double dt = double(now - last) / double(SDL_GetPerformanceFrequency());
        last = now;
        if (!redraw && !animate) continue;

        SDL_GetRendererOutputSize(renderer, &outW, &outH);
        const int fbW = std::max(320, outW / scale), fbH = std::max(240, outH / scale);
        if (frame.width() != fbW || frame.height() != fbH) {
            frame = Image(fbW, fbH);
            if (texture) SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, fbW, fbH);
        }
        studio.uiScale = scale;
        studio.frame(frame, input, std::min(dt, 0.1));
        SDL_SetWindowTitle(window, studio.windowTitle().c_str());

        SDL_UpdateTexture(texture, nullptr, frame.pixels().data(), fbW * 4);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        input.clearEvents();
        if (pendingRelease) { input.mouseDown = false; input.released = true; redraw = true; }
        else redraw = false;
    }

    SDL_StopTextInput();
    if (texture) SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
