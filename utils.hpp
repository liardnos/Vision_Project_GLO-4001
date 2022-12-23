#pragma once

#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string.h>
#include <functional>
#include <mutex>
#include <bitset>
#include <limits.h>


#ifdef WIN32
typedef unsigned int uint;
#endif

#define DEBUGVAR(x) #x << "=" << x

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

void printProgress(double percentage) {
    float val = (percentage * 100);
    percentage = std::min(percentage, 1.0);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3f2%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}

//class Colorf : public NDVector<float, 4> {
class Colorf {
public:
    Colorf(float r, float g, float b, float a) :
        r(r), g(g), b(b), a(a)
    {}

    Colorf squareIt() const {
        return Colorf(r*r, g*g, b*b, a);
    }

    Colorf rootIt() const {
        return Colorf(std::pow(r, 0.5), std::pow(g, 0.5), std::pow(b, 0.5), a);
    }

    Colorf operator/(float const &a) {
        Colorf root = squareIt();
        return Colorf(root.r/a, root.g/a, root.b/a, root.a).rootIt();
    }

    Colorf operator/(Colorf const &a) {
        Colorf root = squareIt();
        Colorf roota = a.squareIt();

        return Colorf(root.r/roota.a, root.g/roota.g, root.b/roota.b, root.a).rootIt();
    }

    static Colorf weightedMed(Colorf const &a, Colorf const &b) {
        Colorf const roota = a.squareIt();
        Colorf const rootb = b.squareIt();

        return Colorf((roota.r+rootb.r)/2, (roota.r+rootb.r)/2, (roota.r+rootb.r)/2, (roota.a+rootb.a)/2).rootIt();
    }

    /*Colorf operator+(Colorf const &other) const {
        Colorf const roota. = this->rootIt();
        Colorf const b = other.rootIt();
        return Colorf(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a).squareIt();
    }*/

    float r;
    float g;
    float b;
    float a;
};

template <class T>
void radixSort(T *toSort, size_t size, T *buffer) noexcept {
    //T *buffer = (T *)malloc(size*sizeof(T));
    int const buftype = 8;
    int const bufsize = pow(2, buftype);

    if (std::is_signed<T>())
        for (size_t x = 0; x < size; x++)
            *(((uint8_t *)(toSort+x))+sizeof(T)-1) ^= 0b10000000;

    for (size_t i = 0; i < sizeof(T); i++) {
        int counts[bufsize+1] = {0};
        
        // counts bits
        for (size_t x = 0; x < size; x++)
            counts[*(((uint8_t *)(toSort+x))+i)+1]++;

        // some counts
        for (size_t x = 1; x < bufsize; x++)
            counts[x] += counts[x-1];

        // place in bucket
        for (size_t x = 0; x < size; x++)
            buffer[counts[*(((uint8_t *)(toSort+x))+i)]++] = toSort[x];

        T *tmp = toSort;
        toSort = buffer;
        buffer = tmp;

    }

    if (sizeof(T) % 2)
        memcpy(buffer, toSort, size*sizeof(T));

    if (std::is_signed<T>())
        for (size_t x = 0; x < size; x++)
            *(((uint8_t *)(toSort+x))+sizeof(T)-1) ^= 0b10000000;
}

template <class T>
void radixSort(T *toSort, size_t size) noexcept {
    static T *buffer = 0;
    static size_t bufferSize = 0;
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    if (bufferSize < size*sizeof(T)) {
        buffer = (T*)realloc(buffer, size*sizeof(T));
    }
    radixSort(toSort, size, buffer);
}

// use offsetof(TaStruct, leNomDuTruc)
template <class T, class U>
void radixSortObj(T *toSort, size_t size, size_t offset_in_struct, T *buffer) noexcept {
    //T *buffer = (T *)malloc(size*sizeof(T));
    int const buftype = 8;
    int const bufsize = pow(2, buftype);

    if (std::is_signed<U>())
        for (size_t x = 0; x < size; x++)
            *(((uint8_t *)(toSort+x))+offset_in_struct+sizeof(U)-1) ^= 0b10000000;

    for (size_t i = 0; i < sizeof(U); i++) {
        int counts[bufsize+1] = {0};
        
        // counts bits
        for (size_t x = 0; x < size; x++)
            counts[*(((uint8_t *)(toSort+x))+offset_in_struct+i)+1]++;

        // some counts
        for (size_t x = 1; x < bufsize; x++)
            counts[x] += counts[x-1];

        // place in bucket
        for (size_t x = 0; x < size; x++)
            buffer[counts[*(((uint8_t *)(toSort+x))+offset_in_struct+i)]++] = toSort[x];
        
        T *tmp = toSort;
        toSort = buffer;
        buffer = tmp;
    }

    if (sizeof(U) % 2)
        memcpy(buffer, toSort, size*sizeof(T));

    if (std::is_signed<U>())
        for (size_t x = 0; x < size; x++)
            *(((uint8_t *)(toSort+x))+offset_in_struct+sizeof(U)-1) ^= 0b10000000;
}

// use offsetof(TaStruct, leNomDuTruc)
template <class T, class U>
void radixSortObj(T *toSort, size_t size, size_t offset_in_struct) noexcept {
    static T *buffer = 0;
    static size_t bufferSize = 0;
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    if (bufferSize < size*sizeof(T)) {
        buffer = (T*)realloc(buffer, size*sizeof(T));
        bufferSize = size*sizeof(T);
    }
    radixSortObj<T, U>(toSort, size, offset_in_struct, buffer);
}

template <class T, uint D>
class NDVector {
public:
    bool rectIntersect(NDVector<T, D> const &other) const {
        for (uint i = 0; i < D/2; ++i)
            if (std::abs((other[i]+std::abs(other[i+D/2]/2)) - ((*this)[i]+std::abs((*this)[i+D/2]/2))) > (std::abs(other[i+D/2]/2) + std::abs((*this)[i+D/2]/2)))
                return false;
        return true;
    }

    int operator==(NDVector<T, D> const &other) const {
        uint count = 0;
        for (uint i = 0; i < D; ++i) 
            count += other[i] == (*this)[i];
        return count;
    }
    
    NDVector<T, D> operator+(NDVector<T, D> const &other) const {
        NDVector<T, D> tmp;
        for (uint i = 0; i < D; ++i) 
            tmp[i] = data[i] + other.data[i];
        return tmp;
    }
    
    NDVector<T, D> operator-(NDVector<T, D> const &other) const {
        NDVector<T, D> tmp;
        for (uint i = 0; i < D; ++i) 
            tmp[i] = data[i] - other.data[i];
        return tmp;
    }
    
    NDVector<T, D> operator*(NDVector<T, D> const &other) const {
        NDVector<T, D> tmp;
        for (uint i = 0; i < D; ++i) 
            tmp[i] = data[i] * other.data[i];
        return tmp;
    }
    
    NDVector<T, D> operator/(NDVector<T, D> const &other) const {
        NDVector<T, D> tmp;
        for (uint i = 0; i < D; ++i) 
            tmp[i] = data[i] / other.data[i];
        return tmp;
    }
    
    NDVector<T, D> operator%(NDVector<T, D> const &other) const {
        NDVector<T, D> tmp;
        for (uint i = 0; i < D; ++i) 
            tmp[i] = data[i] % other.data[i];
        return tmp;
    }
    
    NDVector<T, D> operator+(T const &val) const {
        NDVector<T, D> tmp;
        for (uint i = 0; i < D; ++i) 
            tmp[i] = data[i] + val;
        return tmp;
    }
    
    NDVector<T, D> operator-(T const &val) const {
        NDVector<T, D> tmp;
        for (uint i = 0; i < D; ++i) 
            tmp[i] = data[i] - val;
        return tmp;
    }
    
    NDVector<T, D> operator*(T const &val) const {
        NDVector<T, D> tmp;
        for (uint i = 0; i < D; ++i) 
            tmp[i] = data[i] * val;
        return tmp;
    }
    
    NDVector<T, D> operator/(T const &val) const {
        NDVector<T, D> tmp;
        for (uint i = 0; i < D; ++i) 
            tmp[i] = data[i] / val;
        return tmp;
    }
    
    NDVector<T, D> operator%(T const &val) const {
        NDVector<T, D> tmp;
        for (uint i = 0; i < D; ++i) 
            tmp[i] = data[i] % val;
        return tmp;
    }
    
    NDVector<T, D> &operator+=(NDVector<T, D> const &other) {
        for (uint i = 0; i < D; ++i) 
            data[i] += other.data[i];
        return *this;
    }
    
    NDVector<T, D> &operator-=(NDVector<T, D> const &other) {
        for (uint i = 0; i < D; ++i) 
            data[i] -= other.data[i];
        return *this;
    }
    
    NDVector<T, D> &operator*=(NDVector<T, D> const &other) {
        for (uint i = 0; i < D; ++i) 
            data[i] *= other.data[i];
        return *this;
    }
    
    NDVector<T, D> &operator/=(NDVector<T, D> const &other) {
        for (uint i = 0; i < D; ++i) 
            data[i] /= other.data[i];
        return *this;
    }
    
    NDVector<T, D> &operator%=(NDVector<T, D> const &other) {
        for (uint i = 0; i < D; ++i) 
            data[i] %= other.data[i];
        return *this;
    }
    
    NDVector<T, D> &operator+=(const T &val) {
        for (uint i = 0; i < D; ++i) 
            data[i] += val;
        return *this;
    }
    
    NDVector<T, D> &operator-=(const T &val) {
        for (uint i = 0; i < D; ++i) 
            data[i] -= val;
        return *this;
    }
    
    NDVector<T, D> &operator*=(const T &val) {
        for (uint i = 0; i < D; ++i) 
            data[i] *= val;
        return *this;
    }
    
    NDVector<T, D> &operator/=(const T &val) {
        for (uint i = 0; i < D; ++i) 
            data[i] /= val;
        return *this;
    }
    
    NDVector<T, D> &operator%=(const T &val) {
        for (uint i = 0; i < D; ++i) 
            data[i] %= val;
        return *this;
    }

    T const &operator[](const int i) const {
        return data[i];
    }

    T &operator[](const int i) {
        return data[i];
    }

    T dotProduct(const NDVector<T, D> &other) const {
        T some = data[0]*other.data[0]; 
        for (uint i = 1; i < D; ++i) 
            some += data[i]*other.data[i]; 
        return some;
    }

    T crossProduct(const NDVector<T, 2> &other) const {
        return this->data[0]*other[1] - this->data[1]*other[0];
    }

    NDVector<T, 3> crossProduct(NDVector<T, 3> const &v2) const {
		NDVector<T, 3> v;
		v[0] = (*this)[1] * v2[2] - (*this)[2] * v2[1];
		v[1] = (*this)[2] * v2[0] - (*this)[0] * v2[2];
		v[2] = (*this)[0] * v2[1] - (*this)[1] * v2[0];
		return v;
	}

    template <class R = double>
    double length() const { 
        return pow((R)lengthSquare(), 0.5);
    }

    /*template <class T = NDVector<T, D>, class J>
    NDVector<T, D> pow(J const &pow) const {
        NDVector<T, D> vec;
        for (uint i = 0; i < D; ++i)
            vec[i] = data[i].pow(pow);
        return vec;
    }

    template <class J>
    NDVector<T, D> pow(J const &pow) const {
        NDVector<T, D> vec;
        for (uint i = 0; i < D; ++i)
            vec[i] = std::pow(data[i], pow);
        return vec;
    }*/

    T lengthSquare() const { 
        T some = data[0]*data[0];
        for (uint i = 1; i < D; ++i) 
            some += data[i]*data[i]; 
        return some;
    }
    template <class J = double>
    NDVector<J, D> normalize() const {
		return this->cast<J>() / length();
    }

    NDVector<T, D> &self() {
        return *this;
    }

    template <typename R = double>
    R angle(uint a = 0, uint b = 1) const {
        R a1 = acosf(data[a] / std::pow((data[a]*data[a] + data[b]*data[b]), 0.5));
        if (data[b] < 0)
            a1 = -a1;
        return a1;
    }

    template <typename Tn>
    NDVector<Tn, D> cast() const {
        NDVector<Tn, D> ret;
        for (uint i = 0; i < D; ++i)
            ret.data[i] = (Tn)data[i];
        return ret;
    } 

    NDVector<T, D> round() const {
        NDVector<T, D> ret;
        for (uint i = 0; i < D; ++i)
            ret.data[i] = (T)::round(data[i]);
        return ret;
    }

    // round and cast
    template <typename Tn>
    NDVector<Tn, D> rCast() const {
        NDVector<Tn, D> ret;
        for (uint i = 0; i < D; ++i)
            ret.data[i] = (Tn)::round(data[i]);
        return ret;
    } 

    // same as vec[start:Dn] in python
    template <uint Dn>
    NDVector<T, Dn> truncate(uint const start = 0) const {
        NDVector<T, Dn> vec;
        T *ptr = vec.data;

        for (uint i = start; i < start + Dn; ++i)
            *(ptr++) = data[i];

        return vec;
    }

    T data[D];
};

static inline std::size_t rotl32(std::size_t n, unsigned int c)
{
  const std::size_t mask = (CHAR_BIT*sizeof(n) - 1);
  c &= mask;
  return (n<<c) | (n>>( (-c)&mask ));
}

namespace std {
    template <typename T, uint D>
    struct hash<NDVector<T, D>> {
        std::size_t operator()(NDVector<T, D> const &k) const {
            std::size_t hash = 0;
            uint offset = (sizeof(hash)*8)/D;
            for (uint i = 0; i < D; i++) {
                hash = rotl32(hash^k[i], offset);
            }
            return hash;
        }
    };
}

template <typename T>
using Vector2 = NDVector<T, 2>;

template <typename T, uint D>
std::ostream &operator<<(std::ostream &stream, const NDVector<T, D> &that) {
    stream << typeid(T).name() << D << "{";
        std::cout << that.data[0];
    for (uint i = 1; i < D; ++i)
        std::cout << ", " << that.data[i];
    std::cout << "}";
    return stream;
}


class relativePos {
public:
    uint32_t off;
    uint32_t colide;
};



template <class T, uint D>
class Zone {
public:
    Zone() {}

    explicit Zone(NDVector<T, D> const pos, NDVector<T, D> const size) :
        pos(pos), size(size) {}
    explicit Zone(NDVector<T, D> const pos) :
        pos(pos) {}
        
    // check if the two zone intersect
    bool intersect(Zone const &other) const {
        for (uint i = 0; i < D; ++i)
            if (std::abs(other.pos[i] - pos[i]) > (size[i] + other.size[i]))
                return false;
        return true;
    }

    // check if one of the two zone dimension intersect
    bool intersectOnAnyD(Zone const &other) const {
        for (uint i = 0; i < D; ++i)
            if (std::abs(other.pos[i] - pos[i]) <= (size[i] + other.size[i]))
                return true;
        return false;
    }

    // return an int with the direction of the things and a bool if intersect
    relativePos cmp(Zone const &other) const {
        relativePos cmp = {0, false};
        for (uint i = 0; i < D; ++i) {
            T d = other.pos[i] - pos[i];
            cmp.off |= (uint32_t)(d > 0) << i;
            cmp.colide |= (uint32_t)(std::abs(d) < (size[i] + other.size[i])) << i;
        }
        return cmp;
    }
    
    NDVector<T, D> pos;
    NDVector<T, D> size;
};


template <class T, uint D>
std::ostream& operator<<(std::ostream& stream, Zone<T, D> const &zone) {
    stream << D << typeid(T).name() << "{{";
    stream << zone.pos[0];
    for (uint i = 1; i < D; ++i)
        stream << ", " << zone.pos[1];
    stream << "}, {";
    stream << zone.size[0];
    for (uint i = 1; i < D; ++i)
        stream << ", " << zone.size[1];
    stream << "}}";
    return stream;
}

template <typename T>
class Segment {
public:
    Segment() {}
    Segment(const T x, const T y, const T dx, const T dy) : 
        _p({x, y}), _d({dx, dy})
    {}
    Segment(const Vector2<T> p, const Vector2<T> d) : 
        _p(p), _d(d)
    {}

    double length() const { 
        return _d.length();
    }
    T lengthSquare() const { 
        return _d.lengthSquare();
    }
    double angle() const {
        return _d.angle();
    }

    T operator[](const int i) const {
        return _data[i];
    }

    T &operator[](const int i) {
        return _data[i];
    }

    // check if a point is in the rectangle
    bool isInside(const Vector2<T> &p) const {
        const Vector2<T> p2 = p - _p;
        if (p2[0] >= 0 && p2[0] <= dx && p2[1] >= 0 && p2[1] <= dy)  
            return true;
        else 
            return false;
    }

    Vector2<double> closestPoint(const Vector2<T> p) const {
        const double l2 = lengthSquare();
        if (l2 == 0) return {(double)_p[0], (double)_p[1]};
        const double t = std::max((double)0.0, std::min((double)1.0, (p - _p).dotProduct(_d) / l2));
        return Vector2<double>((double)_p[0], (double)_p[1]) + Vector2<double>((double)_d[0], (double)_d[1]) * t;
    }

    double closestPointDistance(const Vector2<T> p) const {
        const double l2 = lengthSquare();
        if (l2 == 0) return (_p-p).length();
        const double t = std::max((double)0.0, std::min((double)1.0, (p - _p).dotProduct(_d) / l2));
        const Vector2<double> proj = Vector2<double>{(double)_p[0], (double)_p[1]} + Vector2<double>{(double)_d[0], (double)_d[1]} * t;
        return (proj-Vector2<double>{(double)p[0], (double)p[1]}).length();
    }

    //return the t value for the First Segment
    double intersectT(const Segment<T> &seg2) const
    {
        return ((_p[0]*-seg2._d[1] + _p[1]*seg2._d[0] + seg2._p[0]*(seg2._p[1]+seg2._d[1]) - seg2._p[1]*(seg2._p[0]+seg2._d[0])) / (double)(_d[0]*seg2._d[1] - _d[1]*seg2._d[0]));
    }

    T areNotParallel(const Segment<T> &seg2) const {
        return (_d[0]*seg2._d[1] - _d[1]*seg2._d[0]);
    }

    bool isOn(Vector2<T> p) const {
        p -= _p;
        if (p.lengthSquare() >= _d.lengthSquare())
            return false;
        if (p.normalize() == _d.normalize())
            return true;
        return false;
    }

    //// calc t from intersect point and seg // does not do any check
    double intersectT(const Vector2<double> &pos) const
    {
        double x = pos[0] - this->x;
        double y = pos[1] - this->y;
        double dSquare = (this->dx + this->dy);
        double t2 = (x+y)/dSquare;
        return t2;
    }

    // calc intersect pose from t and segments
    Vector2<double> intersectVector2(double t1) const
    {
        return {(double)_p[0] + _d[0]*t1, (double)_p[1] + _d[1]*t1};
    }
    //// calculate the intersect pose
    // do 2 step at once
    Vector2<double> intersectVector2(const Segment<T> &seg2) const
    {
        return intersectVector2(intersectT(seg2));
    }
    // calc t2 from seg1, seg2 and t1
    double intersectTOther(const Segment<T> &seg2, double t1) const
    {
        (void)t1;
        return seg2.intersectT(*this);
    }
    //angle of intersection
    double intersectAngle(const Segment<T> &seg2) const 
    {
        double a1 = acosf(this->_d[0] / this->length());
        if (this->_d[1] < 0)
            a1 = -a1;
        double a2 = acosf(seg2._d[0] / seg2.length());
        if (seg2._d[1] < 0)
            a2 = -a2;
        return a2 - a1;
    }

    Segment<T> round() const {
        return Segment<T>(_p.round(), (_p+_d).round()-_p.round());
    }

    Segment<T> &self() {
        return *this;
    }

    template <typename Tn>
    Segment<Tn> cast() const {
        return Segment<Tn>((Tn)x, (Tn)y, (Tn)dx, (Tn)dy);
    }

    // round and cast
    template <typename Tn>
    Segment<Tn> rCast() const {
        return Segment<Tn>((Tn)::round(x), (Tn)::round(y), (Tn)::round(dx), (Tn)::round(dy));
    }

    union {
        struct {
            Vector2<T> _p;
            Vector2<T> _d;
        };
        struct {
            T x, y, dx, dy;
        };
        T _data[4];
    };
};

typedef Segment<float> Segmentf;
typedef Segment<double> Segmentd;
typedef Segment<int> Segmenti;

template <typename T>
std::ostream &operator<<(std::ostream &stream, const Segment<T> &that) {
    stream <<typeid(T).name() << "{" << that._p << ", " << that._d << "}";
    return stream;
}
