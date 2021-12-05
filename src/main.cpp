#include <iostream>
#include <pixel.hpp>
#include <deque>
#include <unordered_set>
#include "helpmsg.hpp"

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

void parseArg(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-'){
            std::cout << "Unknown argument: " << argv[i] << std::endl;
            exit(1);
        }
        auto len = strlen(argv[i]);
        for (int j = 1; j < len; j++){
            auto option = argv[i][j];
            switch (option){
                default:
                    std::cout << "Unknown option: " << option << std::endl;
                    exit(1);
                case 'h':
                    std::cout << helpmsg << std::endl;
                    exit(0);
            }
        }
    }
}

bool checkLiveOrDie(Pixel::Coor c, Map& map){
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
            return false;

        // Any live cell with two or three live neighbours survives.
        else if (map.at(c))
            return true;

        // Any dead cell with three live neighbours becomes a live cell.
        else if (!map.at(c) && neighbours == 3)
            return true;
        return false;
}

typedef std::unordered_set<Pixel::Coor, Pixel::Coor::Hash> Cells;

#define isPressed(k_) (keyStates[SDL_SCANCODE_##k_])

void keyboardCommands(SDL_Event e, Map& map, Cells& cells, bool& edit, int& fastForward, const uint8_t* keyStates){
    Pixel::Coor c;
    bool shift = isPressed(LSHIFT) || isPressed(RSHIFT);
    bool space = isPressed(SPACE);
    auto window = p->getWindow();
    int newH, newW, newP;
    switch (e.type){
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
                case SDL_SCANCODE_F: 
                    if (shift) fastForward = 0;
                    else fastForward++;
                    break;

                // back to origin
                case SDL_SCANCODE_O:
                    map.camera = { 0, 0 };
                    break;

                // enlarge and shrink pixels
                case SDL_SCANCODE_M:
                case SDL_SCANCODE_N:
                    newP = e.key.keysym.scancode == SDL_SCANCODE_M ?
                                window.pixelw+1 :
                                window.pixelw == 1 ?
                                    1 :
                                    window.pixelw-1;
                    newH = floor((float)window.h*window.pixelw/newP);
                    newW = floor((float)window.w*window.pixelw/newP);
                    p->resizeWindow(newH, newW, newP);
                    break;
            }
            break;
    }

    // move camera
    if (isPressed(W))      map.d_camera.y = -(1+shift*2);
    else if (isPressed(S)) map.d_camera.y =  1+shift*2;
    else                   map.d_camera.y =  0;
    if (isPressed(A))      map.d_camera.x = -(1+shift*2);
    else if (isPressed(D)) map.d_camera.x =  1+shift*2;
    else                   map.d_camera.x =  0;

    // edit
    if (edit && space){
        SDL_GetMouseState(&c.x, &c.y);
        auto actual = map.actual(p->fromScreenCoor(c));
        if (shift){
            map.at(actual) = false;
            cells.erase(actual);
        }else{
            map.at(actual) = true;
            cells.insert(actual);
        }
    }
}

int main(int argc, char** argv){
    parseArg(argc, argv);
    p = new Pixel(75, 100, 10, "Game of Life", 0);
    Map map;

    SDL_Event e;
    bool edit = false;
    Cells cells;
    Pixel::Coor c;
    uint32_t timeStamp = SDL_GetTicks();
    uint32_t timeWait = 125;
    int fastForward = 0;

    while(1){
        auto window = p->getWindow();
        uint32_t frameStart = SDL_GetTicks();

        if (edit) SDL_WaitEvent(&e);
        else SDL_PollEvent(&e);

        switch (e.type){
            case SDL_QUIT:
                exit(0);

            case SDL_WINDOWEVENT:
                p->resizeWindow(&e.window, p->getWindow().pixelw);
                break;

            default:
                auto keyStates = SDL_GetKeyboardState(NULL);
                keyboardCommands(e, map, cells, edit, fastForward, keyStates);
        }

        map.camera.y += map.d_camera.y;
        map.camera.x += map.d_camera.x;

        // rules
        auto nextFrameTime = timeStamp + timeWait * pow(2, -fastForward);
        if (!edit && SDL_TICKS_PASSED(SDL_GetTicks(), nextFrameTime)){
            timeStamp = SDL_GetTicks();
            Cells nextGenLive;
            Cells nextGenDie;
            for (auto cell : cells){
                auto buf = 2;
                for (int i = cell.y-buf; i < cell.y+buf; i++){
                    for (int j = cell.x - buf; j < cell.x+buf; j++){
                        Pixel::Coor c  = {i, j};
                        auto isLive = checkLiveOrDie(c, map);
                        if (isLive) nextGenLive.insert(c);
                        else nextGenDie.insert(c);
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
    return 0;
}
