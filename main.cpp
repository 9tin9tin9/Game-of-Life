#include <iostream>
#include <deque>
#include <unordered_set>
#include <bitset>
#include "pixel.hpp"
#include "helpmsg.hpp"

Pixel* p;
const int sectorWidth = sizeof(std::bitset<1>);

struct Config {
    bool edit;
    int fastForward;
    bool pause;
    std::string loadFile;
    std::string saveFile;
    Pixel::Color editColor, runningColor, pauseColor;
    int density;
    bool grid;
};

int floor(int a, int b){
    float x = (float)a/b;
    int i = (int)x;
    return i - ( i > x );
}

// Infinite grid
struct Map{
    // Actual coordinate of top left pixel
    Pixel::Coor camera = {0, 0};
    Pixel::Coor d_camera = {0, 0};

    struct Sector{
        std::vector<std::bitset<sectorWidth>> plane;
        Sector() {
            plane = std::vector<std::bitset<sectorWidth>>(sectorWidth, 0);
        }
        std::bitset<sectorWidth>::reference at(Pixel::Coor c){
            return plane[c.y][c.x];
        }
    };
    std::deque<std::deque<Sector>> map;
    int top, bottom;
    // Boundaries of each row: pair<left, right>
    std::deque<std::pair<int, int>> boundaries;

    Map(){
        map = { { Sector() } };
        top = 0;
        bottom = 1;
        boundaries = { {0, 1} };
    }

    Pixel::Coor actual(Pixel::Coor onScreen, Config config){
        return {
            camera.y+onScreen.y*config.density,
            camera.x+onScreen.x*config.density};
    }

    // Allocate if necessary
    std::pair<Pixel::Coor, Pixel::Coor> _at(const Pixel::Coor c){
        // Allocate
        int my = floor(c.y, sectorWidth);
        int mx = floor(c.x, sectorWidth);
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
        auto a = my-top;
        while(mx < b.first){
            map.at(a).push_front(Sector());
            b.first--;
        }
        while(mx >= b.second){
            map.at(a).push_back(Sector());
            b.second++;
        }
        // Calculate which sector and relative coordinates
        Pixel::Coor sector = { a, mx - b.first };
        Pixel::Coor relative = { c.y - my*sectorWidth, c.x - mx*sectorWidth };
        
        return {sector, relative};
    }

    std::bitset<sectorWidth>::reference at(const Pixel::Coor c) {
        auto offset = _at(c);
        return map[offset.first.y][offset.first.x].at(offset.second);
        // std::bitset<sectorWidth> a (0);
        // return a[0];
    }
};

typedef std::unordered_set<Pixel::Coor, Pixel::Coor::Hash> Cells;

void parseArg(int argc, char** argv, Config& config) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-'){
            std::cout << "Invalid argument: " << argv[i] << std::endl;
            exit(1);
        }
        auto len = strlen(argv[i]);
        for (int j = 1; j < len; j++){
            auto option = argv[i][j];
            switch (option){
                default:
                    std::cout << "Unknown option: " << option << std::endl;
                    exit(1);

                // Print help msg
                case 'h':
                    std::cout << helpmsg << std::endl;
                    exit(0);

                // Load/Save
                case 'l':
                case 's':
                    if (j != len-1){
                        std::cout << "Expects `" << option << "' as the last option in a option string" << std::endl;
                        exit(1);
                    }
                    i++;
                    if (i == argc){
                        std::cout << "Missing file name requested by option `" << option << "'" << std::endl;
                        exit(1);
                    }
                    (option == 'l' ? config.loadFile : config.saveFile) = argv[i];
                    break;
            }
        }
    }
}

// State file format:
//      text file, 1 line = 1 cell coordinates
//      y coordinate and then x coordinate, separated by space
Cells loadStateFromFile(std::string loadFile){
    if (loadFile.size() == 0)
        return {};
    Cells cells;
    std::ifstream f (loadFile);
    if (!f){
        perror(loadFile.c_str());
        exit(1);
    }
    Pixel::Coor c;
    while(f >> c.y >> c.x){
        cells.insert(c);
    }
    if (f.bad()){
        perror(loadFile.c_str());
        f.close();
        exit(1);
    }
    f.close();
    std::cout << "Save state loaded from " << loadFile << std::endl;
    return cells;
}

void saveStateToFile(std::string saveFile, Cells& cells){
    if (cells.size() == 0)
        return;
    std::ofstream f (saveFile);
    if (!f){
        perror(saveFile.c_str());
        exit(1);
    }
    for (auto c : cells){
        if (!(f << c.y << ' ' << c.x << std::endl)) break;
    }
    if (f.bad()){
        perror(saveFile.c_str());
        f.close();
        exit(1);
    }
    std::cout << "Save state wrote to " << saveFile << std::endl;
    f.close();
}

bool checkLiveOrDie(Pixel::Coor c, Map& map){
        auto n = 0;
        n += map.at({c.y-1, c.x-1});
        n += map.at({c.y-1, c.x  });
        n += map.at({c.y-1, c.x+1});
        n += map.at({c.y  , c.x-1});
        n += map.at({c.y  , c.x+1});
        n += map.at({c.y+1, c.x-1});
        n += map.at({c.y+1, c.x  });
        n += map.at({c.y+1, c.x+1});
        auto isLive = map.at(c);
        // Any live cell with two or three live neighbours survives.
        // Any dead cell with three live neighbours becomes a live cell.
        if (isLive && (n == 2 || n == 3) || (!isLive && n == 3))
            return true;
        // Dead cells remains dead.
        return false;

        /*
        if ((n == 2 || n == 3) || (!isLive && n == 3))
            return true;
        return false;
        */
}

void draw(Cells& cells, Map& map, Pixel::Color color, Config& config){
    p->clear();
    auto win = p->getWindow();
    for (auto c : cells){
        p->set(
            {(c.y-map.camera.y)/config.density,
            (c.x-map.camera.x)/config.density},
            color);
    }
    // Color intensity of grid line depends on pixelw
    auto gridLineColor = 227+win.pixelw > 245 ? 245 : 227+win.pixelw;
    p->render(config.grid && win.pixelw > 5, gridLineColor);
}

#define isPressed(k_) (keyStates[SDL_SCANCODE_##k_])

void controls(SDL_Event e, Map& map, Cells& cells, const uint8_t* keyStates, Config& config){
    Pixel::Coor c;
    if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_P){
        config.pause = !config.pause;
        draw(cells, map,
            (config.pause ? config.pauseColor :
            config.edit ? config.editColor :
            config.runningColor),
            config);
    }
    // Command during pause
    if (config.pause){
        switch (e.type){
            case SDL_KEYDOWN:
                switch (e.key.keysym.scancode){
                    default: break;

                    // Save
                    case SDL_SCANCODE_S:
                        saveStateToFile(config.saveFile, cells);
                        break;
                }
                break;
        }
        return;
    }
    bool shift = isPressed(LSHIFT) || isPressed(RSHIFT);
    bool space = isPressed(SPACE);
    auto window = p->getWindow();
    int newH, newW, newP;
    std::pair<int, int> oldSize;
    // Command during not pause
    switch (e.type){
        case SDL_KEYDOWN:
            switch (e.key.keysym.scancode){
                default: break;
                
                // Edit mode
                case SDL_SCANCODE_E: config.edit = !config.edit; break;

                // Clear cells during editing mode
                case SDL_SCANCODE_C:
                    if (config.edit) {
                       for (auto c : cells)
                           map.at(c) = false;
                       cells.clear();
                    }
                    break;

                // Fast forward
                case SDL_SCANCODE_F: 
                    if (!config.edit){
                        if (shift) config.fastForward = 0;
                        else config.fastForward++;
                    }
                    break;

                // Back to origin
                case SDL_SCANCODE_O:
                    map.camera = { 0, 0 };
                    break;

                // Enlarge and shrink pixels
                case SDL_SCANCODE_M:
                case SDL_SCANCODE_N:
                    if (e.key.keysym.scancode == SDL_SCANCODE_M){
                        // further shrinken pixels after pixelw = 1
                        if (config.density > 1){
                            config.density--;
                            newP = 1;
                        }else{
                            newP = window.pixelw+1;
                        }
                    }else{
                        if (window.pixelw == 1){
                            config.density++;
                            newP = 1;
                        }else{
                            newP = window.pixelw-1;
                        }
                    }
                    oldSize = p->getWindowSize();
                    newH = floor((float)oldSize.first/newP);
                    newW = floor((float)oldSize.second/newP);
                    p->resizeWindow(newH, newW, newP);
                    // set window size again to keep original size
                    p->setWindowSize(oldSize.first, oldSize.second);
                    break;

                // Toggle grid lines
                case SDL_SCANCODE_G:
                    config.grid = !config.grid;
                    break;
            }
            break;
    }

    // Move camera
    auto factor = pow(2, config.density-1) * (6 - (window.pixelw > 5 ? 5 : window.pixelw));
    if      (isPressed(W)) map.d_camera.y = -(1+shift*2)*factor;
    else if (isPressed(S)) map.d_camera.y =  (1+shift*2)*factor;
    else                   map.d_camera.y =  0;
    if      (isPressed(A)) map.d_camera.x = -(1+shift*2)*factor;
    else if (isPressed(D)) map.d_camera.x =  (1+shift*2)*factor;
    else                   map.d_camera.x =  0;

    // Edit
    if (config.edit && space){
        SDL_GetMouseState(&c.x, &c.y);
        auto actual = map.actual(p->fromScreenCoor(c), config);
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
    Config config = {
        .edit = true,
        .fastForward = 0,
        .pause = false,
        .loadFile = "",
        .saveFile = "save.cells",
        .editColor = 10,
        .runningColor = 15,
        .pauseColor = 9,
        .density = 1,
        .grid = true,
    };
    parseArg(argc, argv, config);
    p = new Pixel(75, 100, 15, "Game of Life", 0);
    Map map;

    SDL_Event e;
    Cells cells = loadStateFromFile(config.loadFile);
    for (auto c : cells){
        map.at(c) = true;
    }
    draw(cells, map, config.editColor, config);
    Pixel::Coor c;
    uint32_t timeStamp = SDL_GetTicks();
    uint32_t timeWait = 125;

    while(1){
        auto window = p->getWindow();
        uint32_t frameStart = SDL_GetTicks();

        // Event loop
        while(config.pause ? SDL_WaitEvent(&e) : SDL_PollEvent(&e)){
            switch (e.type){
                case SDL_QUIT:
                    exit(0);

                case SDL_WINDOWEVENT:
                    p->resizeWindow(&e.window, p->getWindow().pixelw);
                    break;

                default:
                    auto keyStates = SDL_GetKeyboardState(NULL);
                    controls(e, map, cells, keyStates, config);
            }
        }

        // Update camera
        map.camera.y += map.d_camera.y;
        map.camera.x += map.d_camera.x;

        // Rules
        auto nextFrameTime = timeStamp + timeWait * pow(2, -config.fastForward);
        if (!config.edit && SDL_TICKS_PASSED(SDL_GetTicks(), nextFrameTime)){
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

        Pixel::Color color = config.edit? config.editColor : config.runningColor;
        draw(cells, map, color, config);
        // Limit frame rate to 60 fps
        int timeElapse = 17 - (SDL_GetTicks()-frameStart);
        SDL_Delay(timeElapse > 0 ? timeElapse : 0);
    }
    return 0;
}
