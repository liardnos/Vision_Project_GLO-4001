#include <SDL2/SDL.h>
#include <unistd.h>
#include <map>
#include <unordered_map>
// #include <SDL2/SDL_ttf.h>

#include "main.hpp"

uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (uint32_t)a | (uint32_t)r << 8 | (uint32_t)g << 16 | (uint32_t)b << 24;
}

bool readConfigSpace(uint8_t const *buffer, int x, int y) {
    int index = (y*600 + x)/8;
    int index2 = (y*600 + x)%8;
    return buffer[index] & (1 << index2);
}

bool readConfigSpace(uint8_t const *buffer, NDVector<int, 2> const &pos) {
    return readConfigSpace(buffer, pos[0], pos[1]); 
}



bool writeConfigSpace(uint8_t *buffer, int x, int y) {
    int index = (y*600 + x)/8;
    int index2 = (y*600 + x)%8;
    buffer[index] = buffer[index] ^ (1 << index2);
    return buffer[index] ^ (1 << index2);
}

bool writeConfigSpace(uint8_t *buffer, NDVector<int, 2> const &pos) {
    return writeConfigSpace(buffer, pos[0], pos[1]);
}

struct Herestics {
    float dToObj;
    float dFromOrigin;
};

bool aStar(SDL_Renderer *renderer, uint8_t *configSpace, NDVector<int, 2> const &start, std::vector<NDVector<int, 2>> &path, std::function<float(int x, int y)> const &heuristicFunc) {
    std::unordered_map<NDVector<int, 2>, float> map;
    std::vector<NDVector<int, 2>> nexts = {
        {-1, -1}, {-1,  0}, {-1,  1},
        { 0, -1},           { 0,  1},
        { 1, -1}, { 1,  0}, { 1,  1},
    };
    if (!(readConfigSpace(configSpace, start)))
        return false;

    std::multimap<float, NDVector<int, 2>> cells;
    cells.emplace(0, start);
    map[start] = 1;
    NDVector<int, 2> lastCell;
    while (cells.size()) {
        std::pair<float, NDVector<int, 2>> p = *cells.begin();
        cells.erase(cells.begin());
        float dFromStart = map[p.second];

        for (NDVector<int, 2> const &next : nexts) {
            NDVector<int, 2> nextCell = p.second + next;
            nextCell[0] += 600;
            nextCell[1] += 600;
            nextCell[0] %= 600;
            nextCell[1] %= 600;
            if ((map[nextCell] == 0 || map[nextCell] > dFromStart+(float)next.length()*1) && readConfigSpace(configSpace, nextCell)) {
                
                
                float remainingD = heuristicFunc(nextCell[0], nextCell[1]);
                if (map[nextCell] == 0) {
                    SDL_SetRenderDrawColor(renderer, remainingD*16, 0, 0, 128);
                    SDL_RenderDrawPoint(renderer, p.second[0], p.second[1]);
                }

                map[nextCell] = dFromStart+(float)next.length()*1;
                if (remainingD == 0) {
                    lastCell = nextCell;
                    goto buildPath;
                }
                cells.emplace(dFromStart+(float)next.length()*1 + remainingD*2, nextCell);
            }
        }
    }
    // SDL_RenderPresent(renderer);
    std::cout << "no path" << std::endl;
    return false;
    buildPath:
    NDVector<int, 2> currentCell = lastCell;
    path.emplace_back(currentCell);
    std::cout << "building path" << std::endl;
    std::cout << map.size() << std::endl;
    float min = map[currentCell];
    while (min != 1) {
        NDVector<int, 2> nextBest = {INT_MAX, INT_MAX};
        for (NDVector<int, 2> const &next : nexts) {
            NDVector<int, 2> nextCell = currentCell - next;
            nextCell[0] += 600;
            nextCell[1] += 600;
            nextCell[0] %= 600;
            nextCell[1] %= 600;
            if (readConfigSpace(configSpace, nextCell) && map.find(nextCell) != map.end() && map[nextCell] < min) {
                min = map[nextCell];
                nextBest = nextCell;
            }
        }
        if ((nextBest == NDVector<int, 2>{INT_MAX, INT_MAX})) {
            std::cout << "failde to find minimum" << std::endl;
            break;
        }
        currentCell = nextBest;
        path.emplace_back(currentCell);
    }
    std::cout << "path finded" << std::endl;
    return true;
}



int main() {
    SDL_Window *_window = SDL_CreateWindow(
        "ConfigurationSpaceSolving",                  // window title
        1920-1200-5,           // initial x position
        0,           // initial y position
        600,                               // width, in pixels
        600,                               // height, in pixels
        SDL_WINDOW_OPENGL                  // flags - see below
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    Uint8 const *sdlKeyboardState = SDL_GetKeyboardState(NULL);


    SDL_Window *_window2 = SDL_CreateWindow(
        "ConfigurationSpaceSolving",                  // window title
        1920-600,           // initial x position
        0,           // initial y position
        600,                               // width, in pixels
        600,                               // height, in pixels
        SDL_WINDOW_OPENGL                  // flags - see below
    );

    SDL_Renderer *renderer2 = SDL_CreateRenderer(_window2, -1, SDL_RENDERER_ACCELERATED);
    
    SDL_SetRenderDrawBlendMode(renderer2, SDL_BLENDMODE_BLEND);

    // TTF_Init();
    // SDL_Font *_font = TTF_OpenFont("./font/font.ttf", 24);
    // std::cout << _font << std::endl;
    // if (!_font)
    //     throw "cannot load font";
    Environments env;
    RobotArm robotArm;
    RobotArm robotArmPreview;
    bool Continue = true;

    uint32_t *pixels;
    int pitch;
    uint8_t *configSpace = (uint8_t *)malloc(600*600/sizeof(*configSpace)+1);
    memset(configSpace, 0xFFFFFFFF, 600*600/sizeof(*configSpace)+1);
    std::cout << DEBUGVAR(sizeof(configSpace)) << std::endl;
    SDL_Texture* buffer = SDL_CreateTexture(renderer2, SDL_PIXELFORMAT_BGRA8888, SDL_TEXTUREACCESS_STREAMING,  600, 600);
    {// calculate configSpace
        std::cout << "start config space..." << std::endl;
        SDL_LockTexture(buffer, NULL, (void **)&pixels, &pitch);
        memset(pixels, 0, 600*600*4);

        for (float y = 0; y < 600; y += 0.5) {
            for (float x = 0; x < 600; x += 0.5) {
                robotArm._node->_a = ((float)x)/600 * M_PI*2;
                robotArm._node->_next[0]->_next->_a = ((float)y)/600 * M_PI*2;
                if ((std::abs(robotArm._node->_next[0]->_next->_a) < M_PI*0.95 || std::abs(robotArm._node->_next[0]->_next->_a) > M_PI*1.05) && robotArm.colideWithEnv(env)) {
                } else {
                    int const margin = 0;
                    for (int yy = -margin; yy <= margin; yy++)
                        for (int xx = -margin; xx <= margin; xx++) {
                            int xxx = (xx+(int)std::round(x)+600)%600;
                            int yyy = (yy+(int)std::round(y)+600)%600;
                            readConfigSpace(configSpace, xxx, yyy) ? writeConfigSpace(configSpace, xxx, yyy) : 0;
                            pixels[yyy*600+xxx] = rgba(255, 255, 255, 255);
                        }
                }
            }
        }

        SDL_UnlockTexture(buffer);
        std::cout << "config space OK" << std::endl;
    }

    NDVector<int, 2> startPos = {300, 300};
    std::vector<NDVector<int, 2>> todoPath;

    while (Continue) {
        Uint32 renderFlags = SDL_GetWindowFlags(_window);
        Uint32 render2Flags = SDL_GetWindowFlags(_window2);
        NDVector<int, 2> mousePos;
        SDL_GetMouseState(&mousePos[0], &mousePos[1]);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        bool doNewPath = false;
        SDL_PumpEvents();
        SDL_Event e;
        while (SDL_PollEvent(&e) > 0) {
            if (e.type == SDL_QUIT) {
                Continue = false;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) { 
                SDL_MouseButtonEvent &m = e.button;
                (void)m.x, (void)m.y;
                if (m.button == SDL_BUTTON_RIGHT) {
                    // if (render2Flags & SDL_WINDOW_MOUSE_FOCUS)
                        doNewPath = true;

                } else if (m.button == SDL_BUTTON_LEFT) {
                    if (render2Flags & SDL_WINDOW_MOUSE_FOCUS) {
                        robotArm._node->_a = ((float)mousePos[0])/600*M_PI*2;
                        robotArm._node->_next[0]->_next->_a = ((float)mousePos[1])/600*M_PI*2;
                    }
                }
            } else if (e.type == SDL_KEYDOWN) {
                SDL_KeyboardEvent &m = e.key;
                std::cout << m.keysym.scancode << std::endl;
                if (m.keysym.scancode == 15) {
                }
            }
        }


        // draw robotArm
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        env.draw(renderer);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        robotArm.draw(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        robotArm.forEachSegments(
            [&env, &renderer] (Segmentf const &seg) -> bool {
                NDVector<float, 2> d = {std::abs(seg._d[0]), std::abs(seg._d[1])};
                std::shared_ptr<std::vector<std::shared_ptr<UniTreeZone<float, Segmentf, 2>::Storage>>> res = env._uniTreeZone.getColides(Zone<float, 2>{seg._p+seg._d/2, d/2});
                for (auto const &ptr : *res) {
                    Segmentf const seg2 = *ptr->_data;
                    float t1 = seg2.intersectT(seg);
                    float t2 = seg.intersectT(seg2);
                    if ( 0 <= t1 && t1 <= 1 && 0 <= t2 && t2 <= 1 ) {
                        NDVector<float, 2> const pos2 = seg._p + seg._d;
                        SDL_RenderDrawLine(renderer, seg._p[0], seg._p[1], pos2[0], pos2[1]);
                        NDVector<float, 2> const pos3 = seg2._p + seg2._d;
                        SDL_RenderDrawLine(renderer, seg2._p[0], seg2._p[1], pos3[0], pos3[1]);
                        return false;
                    }
                }
                return true;
            }
        );

        SDL_RenderCopy(renderer2, buffer, NULL, NULL);

        SDL_SetRenderDrawColor(renderer2, 255, 0, 0, 50);

        std::vector<NDVector<int, 2> > path;
        bool pathExist = false;
        if (!todoPath.size()) {
            if (render2Flags & SDL_WINDOW_MOUSE_FOCUS){
                pathExist = aStar(renderer2, configSpace, startPos, path, [&mousePos](int x, int y) -> float {
                    return (mousePos - NDVector<int, 2>{x, y}).length();
                });
                SDL_RenderDrawLine(renderer2, startPos[0], startPos[1], mousePos[0], mousePos[1]);
            } else if (renderFlags & SDL_WINDOW_MOUSE_FOCUS) {
                RobotArm tmp;
                pathExist = aStar(renderer2, configSpace, startPos, path, [&mousePos, &tmp](int x, int y) -> float {
                    tmp._node->_a = ((float)x)/600*M_PI*2;
                    tmp._node->_next[0]->_next->_a = ((float)y)/600*M_PI*2;
                    NDVector<float, 2> currentPos = tmp._node->_next[0]->_next->_next[0]->_next->pos() + tmp._pos;

                    float d = (mousePos - currentPos.cast<int>()).length();
                    if (d < 2) return 0;
                    return d;
                });
            }
    }

        if (pathExist) {
            if (doNewPath)
                todoPath = path;
            SDL_SetRenderDrawColor(renderer2, 255, 0, 0, 255);
            std::cout << "path finded" << std::endl;
            std::cout << DEBUGVAR(path.size()) << std::endl;
            for (NDVector<int, 2> const &p : path) {
                SDL_RenderDrawPoint(renderer2, p[0], p[1]);
            }
        }


        if (render2Flags & SDL_WINDOW_MOUSE_FOCUS) {
            int x, y;
            SDL_GetMouseState(&x, &y);
            robotArmPreview._node->_a = ((float)x)/600*M_PI*2;
            robotArmPreview._node->_next[0]->_next->_a = ((float)y)/600*M_PI*2;
            SDL_SetRenderDrawColor(renderer2, 0, 255, 0, 255);
            robotArmPreview.draw(renderer);
        }

        if (todoPath.size()) {
            NDVector<int, 2> const p = todoPath[todoPath.size()-1];
            std::cout << DEBUGVAR(p) << std::endl;
            robotArm._node->_a = ((float)p[0])/600*M_PI*2;
            robotArm._node->_next[0]->_next->_a = ((float)p[1])/600*M_PI*2;
            todoPath.resize(todoPath.size()-1);
        }


        int x = (int)(robotArm._node->_a/(M_PI*2)*600)%600;
        int y = (int)(robotArm._node->_next[0]->_next->_a/(M_PI*2)*600)%600;
        startPos = NDVector<int, 2>{x, y};

        {
            NDVector<int, 2> nodePos = (robotArm._node->pos() + robotArm._pos).cast<int>();
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_Rect rect1 = {nodePos[0]-2, nodePos[1]-2, 5, 5};
            SDL_RenderFillRect(renderer, &rect1);
        } {
            NDVector<int, 2> nodePos = (robotArm._node->_next[0]->_next->pos() + robotArm._pos).cast<int>();
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_Rect rect1 = {nodePos[0]-2, nodePos[1]-2, 5, 5};
            SDL_RenderFillRect(renderer, &rect1);
            std::cout << DEBUGVAR(nodePos) << std::endl;
        } {
            NDVector<int, 2> nodePos = (robotArm._node->_next[0]->_next->_next[0]->_next->pos() + robotArm._pos).cast<int>();
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            SDL_Rect rect1 = {nodePos[0]-2, nodePos[1]-2, 5, 5};
            SDL_RenderFillRect(renderer, &rect1);
            std::cout << DEBUGVAR(nodePos) << std::endl;
        }


        SDL_SetRenderDrawColor(renderer2, 255, 0, 0, 255);
        SDL_Rect rect = {x-2, y-2, 5, 5};
        SDL_RenderFillRect(renderer2, &rect);

        // render 2
        SDL_RenderPresent(renderer2);
        //

        // render 1
        SDL_RenderPresent(renderer);
        usleep(1E6/60);
        // if (pathExist)
        //     usleep(3E6);

        //
    }
    return 0;
}