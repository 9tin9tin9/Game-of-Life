#include "pixel.hpp"
#include <string>
#include <thread>
#include <iostream>


Pixel::Error::Error(std::string s) : str(s) {}

bool Pixel::RGB::operator==(RGB rgb)
{
    return r == rgb.r && g == rgb.g && b == rgb.b;
}

bool Pixel::Coor::operator==(const Coor& coor) const {
    return this->y == coor.y && this->x == coor.x;
}

size_t Pixel::Coor::Hash::operator()(const Coor& coor) const {
    size_t xHash = std::hash<int>()(coor.x);
    size_t yHash = std::hash<int>()(coor.y) << 1;
    return xHash ^ yHash;
}

// ostream << Error
std::ostream& operator<<(std::ostream&, Pixel::Error r)
{
    return std::cout << "Pixel::Error: " << r.str;
}

size_t Pixel::Window::convert(Coor c)
{
    return c.y * w + c.x;
}

size_t Pixel::Window::size()
{
    return h*w;
}

size_t Pixel::Window::actualSize()
{
    return h*w*pixelw;
}

void initPalate(std::vector<Pixel::RGB>& palate)
{
    // 16 colors
    Pixel::RGB c16[16] = {
        {0, 0, 0}, {128,0,0}, {0,128,0}, {128,128,0},
        {0,0,128}, {128,0,128}, {0,128,128}, {192,192,192},
        {128,128,128}, {255,0,0}, {0,255,0}, {255,255,0},
        {0,0,255}, {255,0,255}, {0,255,255}, {255,255,255},
    };
    for (int i = 0; i < 16; i++) 
        palate.push_back(c16[i]);

    // middle
    unsigned char ref[6] = { 0, 95, 135, 175, 215, 255 };
    for (short i = 0; i < 6; i++)
        for (short j = 0; j < 6; j++)
            for (short k = 0; k < 6; k++)
                palate.push_back({ ref[i], ref[j], ref[k] });
            
    // black to white
    for (short i = 0; i < 24; i++)
    {
        unsigned char v = 8 + i*10;
        palate.push_back({ v, v, v });
    }
}

void Pixel::drawPixel(Coor c, int h, int w, Color color)
{
    if (c.y < 0 || c.y >= window.h || c.x < 0 || c.x >= window.w)
        return;
    if (h == -1) h = window.h;
    if (w == -1) w = window.w;
    if (c.x + w >= window.w) w = window.w - c.x;
    if (c.y + h >= window.h) h = window.h - c.y;
    if (color == transpaerent) return;
    RGB rgb = palate[color];
    uint32_t clr = *(uint32_t*)&rgb;
    size_t my = c.y*window.pixelw;
    size_t mx = c.x*window.pixelw;
    size_t pww = window.pixelw*window.w;
    size_t mw = w*window.pixelw;
    for (int i = 0; i < h*window.pixelw; i++){
        size_t baseIndex = (my + i)*pww + mx;
        std::fill(&buffer[baseIndex], &buffer[baseIndex + mw], clr);
    }
}

size_t Pixel::onScreenCoor(Coor c){
    return c.y*window.pixelw*window.w*window.pixelw + c.x*window.pixelw;
}

Pixel::Pixel(
    int h,
    int w,
    int pixelWidth,
    std::string name,
    Color base
    ) :
    window({h, w, pixelWidth}), base(base)
{
    if (h < 2 || w < 2 || pixelWidth < 1)
        throw Error("Invalid windows settings");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) 
        throw Error("Couldn't inititalize SDL: "s + SDL_GetError());

    int screenWidth = w * pixelWidth;
    int screenHeight = h * pixelWidth;
    sdl.window = SDL_CreateWindow(
            name.c_str(), 
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            screenWidth,
            screenHeight,
            SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_RESIZABLE);
    if (!sdl.window)
        throw Error("Couldn't create SDL window: "s + SDL_GetError());

    sdl.renderer = SDL_CreateRenderer(
            sdl.window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!sdl.renderer)
        throw Error("Couldn't create SDL renderer: "s + SDL_GetError());

    sdl.texture = SDL_CreateTexture(
            sdl.renderer,
            SDL_PIXELFORMAT_ABGR8888,
            SDL_TEXTUREACCESS_STREAMING,
            screenWidth,
            screenHeight);
    if (!sdl.texture)
        throw Error("Couldn't create SDL texture: "s + SDL_GetError());

    sdl.rect = (SDL_Rect){
                .x = 0, .y = 0,
                .w = screenWidth, .h = screenHeight,
                };

    initPalate(palate);
    buffer = std::vector<uint32_t>(h*w*pixelWidth*pixelWidth, 0);

    // init screen to palate[base]
    SDL_SetRenderDrawColor(sdl.renderer,
            palate[base].r,
            palate[base].g,
            palate[base].b,
            255);
    SDL_RenderClear(sdl.renderer);

    drawPixel({0, 0}, -1, -1, base);
    render(false, 0);
}

Pixel::~Pixel()
{
    SDL_DestroyRenderer(sdl.renderer);
    SDL_DestroyWindow(sdl.window);
}

Pixel::Window Pixel::getWindow()
{
    return window;
}

void Pixel::resizeWindow(int h, int w, int pixelw)
{
    if (h < 1 || w < 1 || pixelw < 1)
        return;
    const auto newScreenHeight = h*pixelw;
    const auto newScreenWidth = w*pixelw;
    std::vector<uint32_t> newBuffer (newScreenHeight*newScreenWidth, 0);
    for (int i = 0; i < window.h; i++){
        for (int j = 0; j < window.w; j++){
            try{
                uint32_t p = buffer.at(onScreenCoor({i, j}));
                for (int k = 0; k < pixelw; k++)
                    for (int l = 0; l < pixelw; l++)
                        try {
                        newBuffer.at((i+k)*newScreenWidth + (j+l)*pixelw) = p;
                        } catch (...) {
                            continue;
                        }
            }catch (...){
                continue;
            }
        }
    }
    buffer = newBuffer;
    window.h = h;
    window.w = w;
    window.pixelw = pixelw;
    SDL_SetWindowSize(sdl.window, newScreenWidth, newScreenHeight);
    SDL_DestroyTexture(sdl.texture);
    sdl.texture = SDL_CreateTexture(
            sdl.renderer,
            SDL_PIXELFORMAT_ABGR8888,
            SDL_TEXTUREACCESS_STREAMING,
            newScreenWidth,
            newScreenHeight);
    sdl.rect.h = newScreenHeight;
    sdl.rect.w = newScreenWidth;
}

void Pixel::resizeWindow(SDL_WindowEvent* e, int pixelw)
{
    if (e->event != SDL_WINDOWEVENT_RESIZED)
        return;
    if (pixelw < 1)
        return;
    int w = floor((float)e->data1 / pixelw);
    int h = floor((float)e->data2 / pixelw);
    resizeWindow(h, w, pixelw);
}

void Pixel::setWindowSize(int h, int w)
{
    SDL_SetWindowSize(sdl.window, w, h);
}

std::pair<int, int> Pixel::getWindowSize()
{
    int h, w;
    SDL_GetWindowSize(sdl.window, &w, &h);
    return {h, w};
}

Pixel::Coor Pixel::fromScreenCoor(Coor c)
{
    return {(int)((c.y+0.5)/window.pixelw), c.x/window.pixelw};
}

void Pixel::set(Coor c, Color color)
{
    if (c.y < 0 || c.y >= window.h || c.x < 0 || c.x >= window.w)
        return;
    drawPixel({c.y, c.x}, 1, 1, color);
}

void Pixel::clear()
{
    std::fill(buffer.begin(), buffer.end(), *(uint32_t*)&palate[base]);
    SDL_SetRenderDrawColor(sdl.renderer, 0, 0, 0, 255);
    SDL_RenderClear(sdl.renderer);
}

void Pixel::render(bool gridlines, Color color)
{
    uint32_t* pixels;
    int pitch;
    SDL_LockTexture(sdl.texture, &sdl.rect, (void**)&pixels, &pitch);
    std::copy(buffer.begin(), buffer.end(), pixels);
    SDL_UnlockTexture(sdl.texture);
    SDL_RenderCopy(sdl.renderer, sdl.texture, &sdl.rect, &sdl.rect);

    if (gridlines){
        const auto mh = window.h*window.pixelw;
        const auto mw = window.w*window.pixelw;
        const auto mmw = window.pixelw * mw;
        auto rgb = palate[color];
        SDL_SetRenderDrawColor(sdl.renderer,
                rgb.r, rgb.g, rgb.b, rgb.a);
        for (int i = 0; i < window.h; i++){
            SDL_RenderDrawLine(sdl.renderer,
                    0, i*window.pixelw,
                    mw, i*window.pixelw);
        }
        for (int i = 0; i < window.w; i++){
            SDL_RenderDrawLine(sdl.renderer,
                    i*window.pixelw, 0,
                    i*window.pixelw, mh);
        }
    }

    SDL_RenderPresent(sdl.renderer);
}
