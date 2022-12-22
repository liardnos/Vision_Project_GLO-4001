#include <SDL2/SDL.h>
#include <unistd.h>
// #include <SDL2/SDL_ttf.h>

#include "main.hpp"

int main() {
    SDL_Window *_window = SDL_CreateWindow(
        "ConfigurationSpaceSolving",                  // window title
        1920/2,           // initial x position
        0,           // initial y position
        800,                               // width, in pixels
        800,                               // height, in pixels
        SDL_WINDOW_OPENGL                  // flags - see below
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    Uint8 const *sdlKeyboardState = SDL_GetKeyboardState(NULL);

    // TTF_Init();
    // SDL_Font *_font = TTF_OpenFont("./font/font.ttf", 24);
    // std::cout << _font << std::endl;
    // if (!_font)
    //     throw "cannot load font";
    Environments env;
    RobotArm robotArm;
    bool Continue = true;
    while (Continue) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        SDL_PumpEvents();
        SDL_Event e;
        while (SDL_PollEvent(&e) > 0) {
            if (e.type == SDL_QUIT) {
                Continue = false;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) { 
                SDL_MouseButtonEvent &m = e.button;
                (void)m.x, (void)m.y;
                if (m.button == SDL_BUTTON_RIGHT) {
                } else if (m.button == SDL_BUTTON_LEFT) {
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
            [&env, &renderer] (Segmentf const &seg) -> void {
                for (auto &seg2 : env._segments) {
                    float t1 = seg2.intersectT(seg);
                    float t2 = seg.intersectT(seg2);
                    if ( 0 <= t1 && t1 <= 1 && 0 <= t2 && t2 <= 1 ) {
                        std::cout << "seg intersect" << std::endl;
                        NDVector<float, 2> const pos2 = seg._p + seg._d;
                        SDL_RenderDrawLine(renderer, seg._p[0], seg._p[1], pos2[0], pos2[1]);
                        NDVector<float, 2> const pos3 = seg2._p + seg2._d;
                        SDL_RenderDrawLine(renderer, seg2._p[0], seg2._p[1], pos3[0], pos3[1]);
                    }
                }

                // std::shared_ptr<std::vector<std::shared_ptr<UniTreeZone<float, Segmentf, 2>::Storage>>> res = env._uniTreeZone.getColides(Zone<float, 2>{seg._p+seg._d/2, seg._d/2});
                // for (auto const &ptr : *res) {
                //     Segmentf const seg2 = *ptr->_data;
                //     float t1 = seg2.intersectT(seg);
                //     float t2 = seg.intersectT(seg2);
                //     if ( 0 <= t1 && t1 <= 1 && 0 <= t2 && t2 <= 1 ) {
                //         std::cout << "seg intersect" << std::endl;
                //         NDVector<float, 2> const pos2 = seg._p + seg._d;
                //         SDL_RenderDrawLine(renderer, seg._p[0], seg._p[1], pos2[0], pos2[1]);
                //         NDVector<float, 2> const pos3 = seg2._p + seg2._d;
                //         SDL_RenderDrawLine(renderer, seg2._p[0], seg2._p[1], pos3[0], pos3[1]);
                //     }
                // }
            }
        );

        robotArm._node->_a += 0.01;
        // robotArm._node->_next[0]->_d += 1;
        robotArm._node->_next[0]->_next->_a += 0.02;
        

        //
        SDL_RenderPresent(renderer);
        usleep(1E6/60);
    }
    return 0;
}