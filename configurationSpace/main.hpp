#include <memory>
#include <SDL2/SDL.h>
#include <vector>
#include <functional>

#include "../utils.hpp"
#include "../UniTree.hpp"
#include "../UniTreeZone.hpp"


bool readConfigSpace(uint8_t const *buffer, int x, int y);
bool writeConfigSpace(uint8_t *buffer, int x, int y);


class Environments {
public:
    Environments() :
        _uniTreeZone(Zone<float, 2>{NDVector<float, 2>{400, 400}, NDVector<float, 2>{400, 400}})
    {
        for (uint i = 0; i < 256; i++) {
            _segments.emplace_back(rand()%800, rand()%800, rand()%100-50, rand()%100-50);
        }
        for (Segmentf &seg : _segments) {
            NDVector<float, 2> d = {std::abs(seg._d[0]), std::abs(seg._d[1])};
            _uniTreeZone.addData(Zone<float, 2>{seg._p+seg._d/2, d/2}, &seg);
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

        bool forEachSegments(std::function<bool(Segmentf const &seg)> const &func, NDVector<float, 2> const &pos, float const &angle) const {
            for (auto const &n : _next)
                if (!n->forEachSegments(func, pos, angle+_a))
                    return false;
            return true;
        }

        NDVector<float, 2> pos() const {
            float a = 0;
            NDVector<float, 2> pos = {0, 0};
            this->pos(pos, a);
            return pos;
        }

        void pos(NDVector<float, 2> &pos, float &a) const {
            if (_prev)
                _prev->pos(pos, a);
            a += _a;
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

        bool forEachSegments(std::function<bool(Segmentf const &seg)> const &func, NDVector<float, 2> const &pos, float const &angle) const {
            NDVector<float, 2> const offset = NDVector<float, 2>{cos(angle)*_d, sin(angle)*_d};
            NDVector<float, 2> const pos2 = pos + offset;
            if (!func(Segmentf{pos, offset}))
                return false;
            else
                return _next->forEachSegments(func, pos2, angle);
        }

        void pos(NDVector<float, 2> &pos, float &a) const {
            _prev->pos(pos, a);
            pos += NDVector<float, 2>{_d * cos(a), _d * sin(a)};
        }

        Node *_prev;
        float _d;
        std::unique_ptr<Node> _next;
    };

    RobotArm() {
        _node = std::make_unique<Node>(nullptr, 0);
        _node->_next.emplace_back(std::make_unique<Segment>(_node.get(), 50));
        _node->_next[0]->_next = std::make_unique<Node>(_node->_next[0].get(), 0);
        _node->_next[0]->_next->_next.emplace_back(std::make_unique<Segment>(_node->_next[0]->_next.get(), 50));
        _node->_next[0]->_next->_next[0]->_next = std::make_unique<Node>(_node->_next[0]->_next->_next[0].get(), 0);
    }

    void draw(SDL_Renderer *renderer) const {
        forEachSegments(
            [renderer](Segmentf const &seg) -> bool {
                NDVector<float, 2> const pos2 = seg._p + seg._d;
                SDL_RenderDrawLine(renderer, seg._p[0], seg._p[1], pos2[0], pos2[1]);
                return true;
            }
        );
    }

    bool forEachSegments(std::function<bool(Segmentf const &seg)> const &func) const {
        return _node->forEachSegments(func, _pos, 0);
    }

    bool colideWithEnv(Environments const &env) const {
        return forEachSegments(
            [&env] (Segmentf const &seg) -> bool {
                NDVector<float, 2> d = {std::abs(seg._d[0]), std::abs(seg._d[1])};
                std::shared_ptr<std::vector<std::shared_ptr<UniTreeZone<float, Segmentf, 2>::Storage>>> res = env._uniTreeZone.getColides(Zone<float, 2>{seg._p+seg._d/2, d/2});
                for (auto const &ptr : *res) {
                    Segmentf const seg2 = *ptr->_data;
                    float t1 = seg2.intersectT(seg);
                    float t2 = seg.intersectT(seg2);
                    if ( 0 <= t1 && t1 <= 1 && 0 <= t2 && t2 <= 1 )
                        return false;
                }
                return true;
            }
        );
    }

    NDVector<float, 2> _pos = {400, 400};
    std::unique_ptr<Node> _node;
};