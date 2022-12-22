#pragma once

#include <vector>
#include <string>
#include <memory>

#include "../utils.hpp"
#include "../mat/mat4.hpp"

template <class T>
class Camera3D {
public:
    Camera3D() {}

    void pointAt(NDVector<T, 3> const &pos, NDVector<T, 3> const &forward, NDVector<T, 3> const &up) {
        _pos = pos;
        _up = up;

        NDVector<double, 3> newForward2 = forward.normalize();
        _forward = newForward2.cast<T>();

        NDVector<T, 3> a = _forward * up.dotProduct(_forward);
        NDVector<double, 3> newUp2 = (up - a).normalize();
        NDVector<T, 3> newUp = newUp2.cast<T>();

        NDVector<T, 3> newRight = newUp.crossProduct(_forward);


        _matTranslateRotate[0*4+0] = newRight[0];
        _matTranslateRotate[0*4+1] = newRight[1];
        _matTranslateRotate[0*4+2] = newRight[2];
        _matTranslateRotate[0*4+3] = 0.0f;

        _matTranslateRotate[1*4+0] = newUp[0];
        _matTranslateRotate[1*4+1] = newUp[1];
        _matTranslateRotate[1*4+2] = newUp[2];
        _matTranslateRotate[1*4+3] = 0.0f;

        _matTranslateRotate[2*4+0] = _forward[0];
        _matTranslateRotate[2*4+1] = _forward[1];
        _matTranslateRotate[2*4+2] = _forward[2];
        _matTranslateRotate[2*4+3] = 0.0f;

        _matTranslateRotate[3*4+0] = pos[0];
        _matTranslateRotate[3*4+1] = pos[1];
        _matTranslateRotate[3*4+2] = pos[2];
        _matTranslateRotate[3*4+3] = 1.0f;
        _matTranslateRotate = _matTranslateRotate.inv();
    }

    void projection(float fFovDegrees, float fAspectRatio, float fNear, float fFar)
	{
		float fFovRad = 1.0f / tanf(fFovDegrees * 0.5f / 180.0f * 3.14159f);
        std::cout << fFovRad << std::endl;
		_matProjection[0*4+0] = fAspectRatio * fFovRad;
		_matProjection[1*4+1] = fFovRad;
		_matProjection[2*4+2] = fFar / (fFar - fNear);
		_matProjection[3*4+2] = (-fFar * fNear) / (fFar - fNear);
		_matProjection[2*4+3] = 1.0f;
		_matProjection[3*4+3] = 0.0f;
        std::cout << "_matProjection" << std::endl;
        std::cout << _matProjection << std::endl;
	}

    void updateValues() {
        _matTranslateRotateInv = _matTranslateRotate.inv();
        _pos = (_matTranslateRotateInv * NDVector<float, 3>{0, 0, 0});
        _forward = (_matTranslateRotateInv * NDVector<float, 3>{0, 0, 1} - _pos);
        _up = (_matTranslateRotateInv * NDVector<float, 3>{0, -1, 0} - _pos);
        std::cout << "_pos=" << _pos << std::endl;
        std::cout << "_forward=" << _forward << std::endl;
        std::cout << "_up=" << _up << std::endl;

        //_matTot =  _matTranslateRotate * _matProjection;
    }

    NDVector<T, 3> cameraRT(NDVector<T, 3> const &val) {
        NDVector<T, 4> res = cameraRT({val[0], val[1], val[2], 1});
        return {res[0], res[1], res[2]};
    }

    NDVector<T, 3> cameraP(NDVector<T, 3> const &val) {
        NDVector<T, 4> res = cameraP({val[0], val[1], val[2], 1});
        return {res[0], res[1], res[2]};
    }

    NDVector<T, 4> cameraRT(NDVector<T, 4> const &val) {
        NDVector<T, 4> res = _matTranslateRotate * val;
        return res;
    }

    NDVector<T, 4> cameraP(NDVector<T, 4> const &val) {
        T tmp = val[2]; 
        NDVector<T, 4> res = val / tmp;
        res[2] = tmp;
        return res;
    }


    void camtt(NDVector<T, 3> const &d) {
        _matTranslateRotate.tt(-d[0], -d[1], -d[2]);
    }

    void camrrx(T const &val) {
        _matTranslateRotate.rrx(val);
    }

    void camrry(T const &val) {
        _matTranslateRotate.rry(val);
    }

    void camrrz(T const &val) {
        _matTranslateRotate.rrz(val);
    }

    void camt(NDVector<T, 3> const &d) {
        _matTranslateRotate.t(-d[0], -d[1], -d[2]);
    }

    void camrx(T const &val) {
        _matTranslateRotate.rx(val);
    }

    void camry(T const &val) {
        _matTranslateRotate.ry(val);
    }

    void camrz(T const &val) {
        _matTranslateRotate.rz(val);
    }

    //std::vector<Triangle<float>> _trianglesBuf;
    NDVector<T, 3> _pos = {0, 0, 0};
    NDVector<T, 3> _forward = {0, 0, 1};
    NDVector<T, 3> _up = {0, -1, 0};
    NDVector<T, 3> _winSize = {0, 0};
    Mat4<T> _matTranslateRotate;
    Mat4<T> _matTranslateRotateInv;
    Mat4<T> _matProjection;
    //Mat4<T> _matTot;
};