#include <map>

#include <opencv2/opencv.hpp>

#include "utils.hpp"
#include "UniTree.hpp"


typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;

NDVector<int, 4> g_paterns[128];

struct Brief {
    static void briefInit();

    Brief(cv::Mat const &img, NDVector<int, 2> const &pos);


    int diff(Brief const &res1) const;

    uint128_t res;
};

class Descriptor : public NDVector<float, 2> {
public:
    Descriptor(cv::Mat const &img, NDVector<int, 2> const &p);

    // NDVector<int, 2> _p;
    Brief _brief;
};

class Frame {
public:
    Frame(std::string const &path);

    void addMatch(NDVector<int, 2> const &p) {
        _descriptors.emplace_back(_image, p);
        // _fastMatch.addData(&_descriptors[_descriptors.size()-1]);
    }

    cv::Mat _image;
    NDVector<int, 2> _size;
    std::vector<Descriptor> _descriptors;
    UniTree<Descriptor, NDVector<float, 2>, 2> _fastMatch;
};