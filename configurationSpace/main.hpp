#include <memory>
#include <SDL2/SDL.h>
#include <vector>
#include <functional>

#include "../utils.hpp"
#include "../UniTree.hpp"
#include "../UniTreeZone.hpp"



class Environments {
public:
    Environments() :
        _uniTreeZone(Zone<float, 2>{NDVector<float, 2>{400, 400}, NDVector<float, 2>{400, 400}})
    {
        for (uint i = 0; i < 512; i++) {
            Segmentf seg = {rand()%800, rand()%800, rand()%100-50, rand()%100-50};
            _segments.emplace_back(seg);
            _uniTreeZone.addData(Zone<float, 2>{seg._p+seg._d/2, seg._d/2}, &_segments[_segments.size()-1]);
        }
    }

    void draw(SDL_Renderer *renderer) const {
        for (Segmentf const &segment : _segments) {
            NDVector<float, 2> pos2 = segment._p + segment._d;
            SDL_RenderDrawLine(renderer, segment[0], segment[1], pos2[0], pos2[1]);
        }
    }

    std::vector<Segmentf> _segments;
    UniTreeZone<float, Segmentf, 2> _uniTreeZone;
};

class RobotArm {
public:
    class Node;
    class Segment;

    class Node {
    public:
        Node(Segment *parent, float a) : 
            _prev(parent), _a(a)
        {}

        // void draw(SDL_Renderer *renderer, NDVector<float, 2> const &pos, float angle) const {
        //     for (auto const &n : _next) {
        //         n->draw(renderer, pos, angle+_a);
        //     }
        // }

        void forEachSegments(std::function<void(Segmentf const &seg)> const &func, NDVector<float, 2> const &pos, float const &angle) const {
            for (auto const &n : _next) {
                n->forEachSegments(func, pos, angle+_a);
            }
        }


        Segment *_prev;
        float _a = 0;
        std::vector<std::unique_ptr<Segment>> _next;
    };

    class Segment {
    public:
        Segment(Node *parent, float d) : _prev(parent), _d(d) {}
        // void draw(SDL_Renderer *renderer, NDVector<float, 2> const &pos, float angle) const {
        //     NDVector<float, 2> const pos2 = pos + NDVector<float, 2>{cos(angle)*_d, sin(angle)*_d};
        //     SDL_RenderDrawLine(renderer, pos[0], pos[1], pos2[0], pos2[1]);
        //     _next->draw(renderer, pos2, angle);
        // }

        void forEachSegments(std::function<void(Segmentf const &seg)> const &func, NDVector<float, 2> const &pos, float const &angle) const {
            NDVector<float, 2> const offset = NDVector<float, 2>{cos(angle)*_d, sin(angle)*_d};
            NDVector<float, 2> const pos2 = pos + offset;
            func(Segmentf{pos, offset});
            _next->forEachSegments(func, pos2, angle);
        }

        Node *_prev;
        float _d;
        std::unique_ptr<Node> _next;
    };

    RobotArm() {
        _node = std::make_unique<Node>(nullptr, 0);
        _node->_next.emplace_back(std::make_unique<Segment>(_node.get(), 50));
        _node->_next[0]->_next = std::make_unique<Node>(_node->_next[0].get(), 0);
        _node->_next[0]->_next->_next.emplace_back(std::make_unique<Segment>(_node.get(), 50));
        _node->_next[0]->_next->_next[0]->_next = std::make_unique<Node>(_node->_next[0].get(), 0);
    }

    void draw(SDL_Renderer *renderer) const {
        forEachSegments(
            [renderer](Segmentf const &seg) -> void {
                NDVector<float, 2> const pos2 = seg._p + seg._d;
                SDL_RenderDrawLine(renderer, seg._p[0], seg._p[1], pos2[0], pos2[1]);
                return;
            }
        );
    }

    void forEachSegments(std::function<void(Segmentf const &seg)> const &func) const {
        _node->forEachSegments(func, _pos, 0);
    }

    NDVector<float, 2> _pos = {400, 400};
    std::unique_ptr<Node> _node;
};