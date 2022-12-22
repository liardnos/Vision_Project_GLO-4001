#include "mesh.hpp"

std::shared_ptr<Mesh> fusionMeshs(std::vector<Mesh *> const &meshs, std::shared_ptr<Mesh> dest) {
    if (!dest)
        dest = std::make_shared<Mesh>();
    uint points_count = 0;
    uint tris_count = 0;
    for (Mesh * const &m : meshs) {
        points_count += m->_points.size();
        tris_count += m->_tris.size();
    }
    dest->_points.resize(points_count);
    dest->_tris.resize(tris_count);
    dest->_meshsPos.resize(meshs.size());
    uint points_offset = 0;
    uint tris_offset = 0;
    uint i = 0;
    for (Mesh * const &m : meshs) {
        dest->_meshsPos[i] = tris_offset;
        uint i_max = m->_points.size();
        for (uint i = 0; i < i_max; i++) {
            dest->_points[points_offset+i] = m->_mat * m->_points[i];
        }
        i_max = m->_tris.size();
        for (uint i = 0; i < i_max; i++) {
            dest->_tris[tris_offset+i] = m->_tris[i];
            dest->_tris[tris_offset+i]._p += points_offset;
        }
        points_offset += m->_points.size();
        tris_offset += m->_tris.size();
        i++;
    }
    dest->_mat = Mat4<float>();
    return dest;
}

void optimiseMesh(Mesh &mesh) {
    std::vector<uint> toRm;
    std::vector<uint> replaceBy;
    for (uint i = 0; i < mesh._points.size(); i++) {
        for (uint j = i+1; j < mesh._points.size(); j++) {
            if(mesh._points[i].data[0] == mesh._points[j].data[0] 
            && mesh._points[i].data[1] == mesh._points[j].data[1]
            && mesh._points[i].data[2] == mesh._points[j].data[2]) {
                toRm.push_back(j);
                replaceBy.push_back(i);
                break;
            }
        }
    }
    for (Mesh::Triangle &tri : mesh._tris) {
        for (uint x = 0; x < 3; x++) {
            uint &id = tri._p[x];
            for (uint i = 0; i < toRm.size(); i++) {
                if (toRm[i] == id) {
                    id = replaceBy[i];
                    break;
                }
            }
        }
    }

    std::vector<uint> refCount(mesh._points.size(), 0);
    for (Mesh::Triangle &tri : mesh._tris) {
        for (int x = 0; x < 3; x++) {
            refCount[tri._p[x]]++;
        }
    }
    uint offset = 0;
    for (uint i = 0; i < refCount.size(); i++) {
        uint &val = refCount[i];
        if (val) {
            val = offset;
            mesh._points[offset] = mesh._points[i];
            offset++;
        } else {
            val = offset;
        }
    }
    mesh._points.resize(offset+1);

    for (Mesh::Triangle &tri : mesh._tris) {
        for (int x = 0; x < 3; x++) {
            tri._p[x] = refCount[tri._p[x]];
        }
    }

    std::cout << "point to rm " << toRm.size() << std::endl;
}