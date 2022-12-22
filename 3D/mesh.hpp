#pragma once

#include <memory>
#include <fstream>
#include <sstream>

#include "../utils.hpp"
#include "../graphicLibs/graphicLibs.hpp"
#include "../mat/mat4.hpp"

class Mesh {
public:
    class Triangle {
    public:
        Triangle() {}
        Triangle(NDVector<uint, 3> const &p, grLib::Color const &color) :
            _p(p), _color(color) 
        {}
        NDVector<uint, 3> _p;
        grLib::Color _color;
    };

    Mesh() {}
    Mesh(std::vector<NDVector<float, 4>> const &points, std::vector<Triangle> const &tris) : 
        _points(points), _tris(tris)
    {}

    void clear() {
        _points.clear();
        _tris.clear();
    }

    Mesh operator+(Mesh const &other) const {
        Mesh newMesh;
        newMesh._tris.reserve(this->_tris.size() + other._tris.size());
        
        for (auto const &p : this->_points)
            newMesh._points.push_back(p);
        for (auto const &p : other._points)
            newMesh._points.push_back(p);

        newMesh._points.reserve(this->_points.size() + other._points.size());
        uint offset = this->_points.size();
        for (auto const &tri : this->_tris)
            newMesh._tris.push_back(tri);
        for (auto const &tri : other._tris) {
            Triangle point = tri;
            point._p += offset;
            newMesh._tris.push_back(point);
        }
        return newMesh;
    }

    bool loadFromObjectFile(std::string const &sFilename) {
        std::ifstream f(sFilename);
        if (!f.is_open())
            return false;

        while (!f.eof()) {
            char line[256];
            f.getline(line, 256);

            std::istringstream s(line);

            char junk;

            if (line[0] == 'v') {
                NDVector<float, 4> v;
                s >> junk >> v[0] >> v[1] >> v[2];
                v[3] = 1;
                _points.push_back(v);
            } else if (line[0] == 'f') {
                uint f[3];
                int color[4];
                s >> junk >> f[0] >> f[1] >> f[2] >> color[0] >> color[1] >> color[2] >> color[3];
                _tris.push_back(
                    {
                        NDVector<uint, 3>{f[0]-1, f[1]-1, f[2]-1}, 
                        grLib::Color{(unsigned char)color[0], (unsigned char)color[1], (unsigned char)color[2], (unsigned char)color[3]}
                    }
                    
                );
            }
        }
        std::cout << "sFilename : " << sFilename << std::endl;
        std::cout << "points    : "  << _points.size() << std::endl;
        std::cout << "tris      : "  << _tris.size() << std::endl;
        return true;
    }

    bool saveInObjectFile(std::string const &sFilename) const {
        std::ofstream f(sFilename);
        if (!f.is_open())
            return false;
        for (NDVector<float, 4> const &v : _points) {
            f << "v " << v[0] << " " << v[1] << " " << v[2] << std::endl;
        }
        for (Triangle const &tri: _tris) {
            f << "f " << tri._p[0]+1 << " " << tri._p[1]+1 << " " << tri._p[2]+1 << " " << (int)tri._color.color[0] << " " << (int)tri._color.color[1] << " " << (int)tri._color.color[2] << " " << (int)tri._color.color[3] << std::endl;
        }
        return true;
    } 

    NDVector<NDVector<float, 3>, 2> getBoundingBox() const {
        NDVector<float, 4> p = _mat * _points[0];
        NDVector<NDVector<float, 3>, 2> boundingBox = {
            p.truncate<3>(),
            p.truncate<3>()
        };

        for (uint i = 1; i < _points.size(); ++i) {
            for (uint j = 0; j < 3; ++j) {
                NDVector<float, 4> p = _mat * _points[i];
                if (p[j] < boundingBox[0][j]) {
                    boundingBox[0][j] = p[j];
                } else if (p[j] > boundingBox[1][j]) {
                    boundingBox[1][j] = p[j];
                }
            }
        }
        return boundingBox;
    }

    Mat4<float> _mat;

    std::vector<NDVector<float, 4>> _points;
    mutable std::vector<NDVector<float, 3>> _transformedPoints;
    std::vector<Triangle> _tris;
    std::vector<uint> _meshsPos = {0};
};

std::shared_ptr<Mesh> fusionMeshs(std::vector<Mesh *> const &meshs, std::shared_ptr<Mesh> dest = 0);
void optimiseMesh(Mesh &mesh);