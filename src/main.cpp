#include <iostream>
#include <pixel.hpp>

Pixel* p;
const size_t WIDTH = 30;

void addCellsToMap(std::vector<std::vector<bool>>& map, std::vector<Pixel::Coor> cells)
{
    for (auto c : cells){
        map[c.y][c.x] = true;
    }
}

struct Map{
    std::vector<std::vector<bool>> map;

    Map(){
        map = std::vector<std::vector<bool>> (WIDTH, std::vector<bool>(WIDTH, false));
    }

    bool at(Pixel::Coor c) {
        if (c.y < 0 || c.y >= WIDTH || c.x < 0 || c.x >= WIDTH)
            return false;
            // throw Pixel::Error("Out of range");
        return map[c.y][c.x];
    }
    void set(Pixel::Coor c, bool val) {
        if (c.y < 0 || c.y >= WIDTH || c.x < 0 || c.x >= WIDTH)
            return;
        map[c.y][c.x] = val;
        if (val) p->set(c, 7);
        else p->set(c, p->base);
    }

};

int main(){
    p = new Pixel(WIDTH, WIDTH, 10, "Game of Life", 0);
    Map map;

    SDL_Event e;
    bool edit = false;
    Pixel::Coor c;
    while(1){
        while(SDL_PollEvent(&e)){
            switch (e.type){
                case SDL_QUIT:
                    goto end;
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (edit){
                        c = p->fromScreenCoor({e.button.y, e.button.x});
                        map.set(c, !map.at(c));
                    }
                    break;

                case SDL_KEYDOWN:
                    if (e.key.keysym.scancode == SDL_SCANCODE_E)
                        edit = !edit;
            }
        }

        // rules
        if (!edit){
            std::vector<Pixel::Coor> nextGenLive;
            std::vector<Pixel::Coor> nextGenDie;
            for (int i = 0; i < WIDTH; i++)
                for (int j = 0; j < WIDTH; j++){
                    Pixel::Coor c = {i, j};
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
                        nextGenDie.push_back(c);

                    // Any live cell with two or three live neighbours survives.
                    else if (map.at(c))
                        nextGenLive.push_back(c);

                    // Any dead cell with three live neighbours becomes a live cell.
                    else if (!map.at(c) && neighbours == 3)
                        nextGenLive.push_back(c);

                }
            for (auto c : nextGenLive)
                map.set(c, true);
            for (auto c : nextGenDie)
                map.set(c, false);
        }

        p->render();
        if (!edit) SDL_Delay(250);
    }
end:
    return 0;
}
