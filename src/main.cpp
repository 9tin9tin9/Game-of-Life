#include <iostream>
#include <pixel.hpp>
#include <deque>
#include <unordered_set>

Pixel* p;
const int sectorWidth = 30;

void addCellsToMap(std::vector<std::vector<bool>>& map, std::vector<Pixel::Coor> cells)
{
    for (auto c : cells){
        map[c.y][c.x] = true;
    }
}

// Infinite grid
struct Map{
    // actual coordinate of top left pixel
    Pixel::Coor camera = {0, 0};
    Pixel::Coor d_camera = {0, 0};

    struct Sector{
        std::vector<std::vector<bool>> plane;
        Sector() {
            plane = std::vector<std::vector<bool>>(sectorWidth, std::vector<bool>(sectorWidth, false));
        }
        std::vector<bool>::reference at(Pixel::Coor c){
            return plane.at(c.y).at(c.x);
        }
    };
    std::deque<std::deque<Sector>> map;
    int top, bottom;
    // pair<left, right>
    std::deque<std::pair<int, int>> boundaries;

    Map(){
        map = { { Sector() } };
        top = 0;
        bottom = 1;
        boundaries = { {0, 1} };
    }

    Pixel::Coor actual(Pixel::Coor onScreen){
        return {camera.y + onScreen.y, camera.x + onScreen.x};
    }

    // allocate if necessary
    std::pair<Pixel::Coor, Pixel::Coor> _at(Pixel::Coor c){
        // allocate
        int my = floor((float)c.y/sectorWidth);
        int mx = floor((float)c.x/sectorWidth);
        while(my < top){
            map.push_front({});
            boundaries.push_front({0, 0});
            top--;
        }
        while(my >= bottom){
            map.push_back({});
            boundaries.push_back({0, 0});
            bottom++;
        }
        auto& b = boundaries.at(my-top);
        while(mx < b.first){
            map.at(my-top).push_front(Sector());
            b.first--;
        }
        while(mx >= b.second){
            map.at(my-top).push_back(Sector());
            b.second++;
        }
        // calculate which sector and relative coordinates
        Pixel::Coor sector = { my - top, mx - b.first };
        Pixel::Coor relative = { c.y - my*sectorWidth, c.x - mx*sectorWidth };
        
        return {sector, relative};
    }

    std::vector<bool>::reference at(Pixel::Coor c) {
        auto offset = _at(c);
        return map.at(offset.first.y).at(offset.first.x).at(offset.second);
    }

};

int main(){
    p = new Pixel(75, 100, 10, "Game of Life", 0);
    auto window = p->getWindow();
    Map map;

    SDL_Event e;
    bool edit = false;
    std::unordered_set<Pixel::Coor, Pixel::Coor::Hash> cells;
    Pixel::Coor c;
    uint32_t timeStamp = SDL_GetTicks();
    uint32_t timeWait = 125;
    bool fastForward = false;

    while(1){
        uint32_t frameStart = SDL_GetTicks();
        auto keyStates = SDL_GetKeyboardState(NULL);
        while(SDL_PollEvent(&e)){
            switch (e.type){
                case SDL_QUIT:
                    goto end;

                case SDL_KEYDOWN:
                    switch (e.key.keysym.scancode){
                        default: break;
                        
                        // edit mode
                        case SDL_SCANCODE_E: edit = !edit; break;
                        // clear cells during edit mode
                        case SDL_SCANCODE_C:
                            if (edit) {
                               for (auto c : cells)
                                   map.at(c) = false;
                               cells.clear();
                            }
                            break;
                        // fast forward
                        case SDL_SCANCODE_F: fastForward = true; break;

                        // move camera
                        case SDL_SCANCODE_W: map.d_camera.y = -1; break;
                        case SDL_SCANCODE_S: map.d_camera.y =  1; break;
                        case SDL_SCANCODE_A: map.d_camera.x = -1; break;
                        case SDL_SCANCODE_D: map.d_camera.x =  1; break;

                        // back to origin
                        case SDL_SCANCODE_O:
                            map.camera = { 0, 0 };
                            break;
                        
                        // edit
                        case SDL_SCANCODE_SPACE:
                            if (edit){
                                SDL_GetMouseState(&c.x, &c.y);
                                auto actual = map.actual(p->fromScreenCoor(c));
                                if (keyStates[SDL_SCANCODE_LSHIFT] || keyStates[SDL_SCANCODE_RSHIFT]){
                                    map.at(actual) = false;
                                    cells.erase(actual);
                                }else{
                                    map.at(actual) = true;
                                    cells.insert(actual);
                                }
                            }
                            break;
                    }
                    break;

                case SDL_KEYUP:
                    switch (e.key.keysym.scancode){
                        default: break;
                        case SDL_SCANCODE_F: fastForward = false;
                        case SDL_SCANCODE_W: case SDL_SCANCODE_S: map.d_camera.y = 0; break;
                        case SDL_SCANCODE_A: case SDL_SCANCODE_D: map.d_camera.x = 0; break;
                    }
            }
        }
        map.camera.y += map.d_camera.y;
        map.camera.x += map.d_camera.x;

        // rules
        auto nextFrameTime = timeStamp + timeWait * pow(2, fastForward? -1 : 1);
        if (!edit && SDL_TICKS_PASSED(SDL_GetTicks(), nextFrameTime)){
            timeStamp = SDL_GetTicks();
            std::unordered_set<Pixel::Coor, Pixel::Coor::Hash> nextGenLive;
            std::unordered_set<Pixel::Coor, Pixel::Coor::Hash> nextGenDie;
            auto top = map.top;
            auto bottom = map.bottom;
            for (auto cell : cells){
                for (int i = cell.y-2; i < cell.y+2; i++){
                    for (int j = cell.x - 2; j < cell.x+2; j++){
                        Pixel::Coor c  = {i, j};
                        auto neighbours = 0;
                        neighbours += map.at({c.y-1, c.x-1});
                        neighbours += map.at({c.y-1, c.x  });
                        neighbours += map.at({c.y-1, c.x+1});
                        neighbours += map.at({c.y  , c.x-1});
                        neighbours += map.at({c.y  , c.x+1});
                        neighbours += map.at({c.y+1, c.x-1});
                        neighbours += map.at({c.y+1, c.x  });
                        neighbours += map.at({c.y+1, c.x+1});
                        // Any live cell with less than two or more than three live neighbours dies;
                        if (neighbours < 2 || neighbours > 3)
                            nextGenDie.emplace(c);

                        // Any live cell with two or three live neighbours survives.
                        else if (map.at(c))
                            nextGenLive.emplace(c);

                        // Any dead cell with three live neighbours becomes a live cell.
                        else if (!map.at(c) && neighbours == 3)
                            nextGenLive.emplace(c);

                    }
                }
            }
            for (auto c : nextGenLive)
                map.at(c) = true;
            
            for (auto c : nextGenDie)
                map.at(c) = false;

            cells = nextGenLive;
        }

        p->clear();
        for (int i = 0; i < window.h; i++){
            for (int j = 0; j < window.w;  j++){
                p->set({i, j}, map.at({map.camera.y + i, map.camera.x + j}) ? edit ? 10 : 15 : 0);
            }
        }
        p->render();
        int timeElapse = 17 - (SDL_GetTicks()-frameStart);
        SDL_Delay(timeElapse > 0 ? timeElapse : 0);
    }
end:
    return 0;
}
