#ifndef _PIXEL_H_
#define _PIXEL_H_

#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <SDL.h>

using namespace std::string_literals;

class Pixel {

    public:
    struct Error {
        std::string str;
        friend std::ostream& operator<<(std::ostream&, Error r);
        Error(std::string);
    };

    typedef short Color;
    const Color transpaerent = -1;

    struct RGB{
        uint8_t r, g, b, a=255;
        bool operator==(RGB rgb);
    };

    struct Coor{
        int y, x;
        bool operator==(const Coor& coor) const;
        struct Hash{
            size_t operator()(const Coor& coor) const;
        };
    };

    private:
    struct Window{
        int h, w;
        int pixelw;
        size_t convert(Coor c);
        size_t size();
        size_t actualSize();
    }window;

    struct SDL{
        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_Texture* texture;
        SDL_Rect rect;
    }sdl;

    std::vector<RGB> palate;
    std::vector<uint32_t> buffer;

    void drawPixel(Coor c, int h, int w, Color color);
    size_t onScreenCoor(Coor c);

    public:
    Color base;

    Pixel(int h, int w, int pixelWidth, std::string name, Color base);
    ~Pixel();

    Window getWindow();
    // resize window by program
    void resizeWindow(int h, int w, int pixelWidth);
    // resize window by event
    void resizeWindow(SDL_WindowEvent* e, int pixelWidth);
    // actual window size, pass to SDL_SetWindowSize()
    void setWindowSize(int h, int w);
    // pair<height, width>
    std::pair<int, int> getWindowSize();
    Coor fromScreenCoor(Coor c);

    void set(Coor c, Color color);
    void clear();

    void refresh();
    void render(bool gridlines, Color color);
};

#endif
