#include <stdio.h>
#include <filesystem>
#include <map>

#include "main.hpp"


void Brief::briefInit() {
    srand(0);
    for (uint i = 0; i < 128; i++) {
        g_paterns[i] = {rand()%63-31, rand()%63-31, rand()%63-31, rand()%63-31};
    }
}

Brief::Brief(cv::Mat const &img, NDVector<int, 2> const &pos) {
    res = 0;
    for (uint i = 0; i < sizeof(res)*8; i++) {
        int p1 = img.at<uint8_t>(pos[1]+g_paterns[i][0], (pos[0]+g_paterns[i][1])*3);
        int p2 = img.at<uint8_t>(pos[1]+g_paterns[i][2], (pos[0]+g_paterns[i][3])*3);
        res <<= 1;
        res |= p1 < p2;
    }
}


int Brief::diff(Brief const &res1) const {
    uint128_t res = res1.res ^ this->res;
    int dif = 0;
    for (uint i = 0; i < sizeof(res)*8; i++)
        dif += (res >> i) & 0x1;
    return dif;
}

Descriptor::Descriptor(cv::Mat const &img, NDVector<int, 2> const &p) : 
    NDVector<float, 2>(p.cast<float>()),
    _brief(img, p)
{}

Frame::Frame(std::string const &path) : 
    _image(cv::imread(path, 1)),
    _size(NDVector<int, 2>{_image.size().width, _image.size().height}),
    _fastMatch(_size.cast<float>()/2+1, _size.cast<float>()/2+2)
{}


std::string type2str(int type) {
  std::string r;

  uint8_t depth = type & CV_MAT_DEPTH_MASK;
  uint8_t chans = 1 + (type >> CV_CN_SHIFT);

  switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
  }

  r += "C";
  r += (chans+'0');

  return r;
}

int main(int argc, char** argv )
{
    if ( argc != 2 )
    {
        printf("usage: ./Vision <directory_path>\n");
        return -1;
    }

    Brief::briefInit();

    std::map<std::string, Frame> images;
    for (const auto & entry : std::filesystem::directory_iterator(argv[1])) {
        std::cout << entry.path() << std::endl;
        images.emplace(entry.path(), entry.path());
    }

    NDVector<int, 2> const fastOrder[16] = {
        { 3, -1},
        { 3,  0},
        { 3,  1},
        { 2,  2},

        { 1,  3},
        { 0,  3},
        {-1,  3},
        {-2,  2},

        {-3,  1},
        {-3,  0},
        {-3, -1},
        {-2, -2},

        {-1, -3},
        { 0, -3},
        { 1, -3},
        { 2, -2},
    };
    NDVector<int, 2> fastSize = {32, 32};

    if (!"correct for sqrt"){
        for (auto & pair : images) {
            cv::Mat &img = pair.second._image;
            Frame &frame = pair.second;
            NDVector<int, 2> imgSize = {img.size().width, img.size().height};
            for (int y = 0; y < imgSize[1]; y++) {
                for (int x = 0; x < imgSize[0]; x++) {
                    img.at<uint8_t>(y, x*3) = (std::pow((float)img.at<uint8_t>(y, x*3), 2))*0.00392156862;
                    img.at<uint8_t>(y, x*3+1) = (std::pow((float)img.at<uint8_t>(y, x*3), 2))*0.00392156862;
                    img.at<uint8_t>(y, x*3+2) = (std::pow((float)img.at<uint8_t>(y, x*3), 2))*0.00392156862;
                }
            }
        }
    }

    for (auto & pair : images) {
        cv::Mat &img = pair.second._image;
        Frame &frame = pair.second;
        std::cout << "|" << std::flush;
        NDVector<int, 2> imgSize = {img.size().width, img.size().height};
        for (int y = fastSize[1]; y < imgSize[1]-fastSize[1]; y++) {
            for (int x = fastSize[0]; x < imgSize[0]-fastSize[0]; x++) {
                int p = img.at<uint8_t>(y, x*3);
                int maxLoop = 0;
                int currentLoop = 0;
                int t = 40;
                // can be optimized by checking corner first
                for (int i = 0; i < 32; i++) {
                    NDVector<int, 2> const &currentPos = fastOrder[i%16];
                    int res = std::abs(p - img.at<uint8_t>((y+currentPos[1]), (x+currentPos[0])*3));
                    if (res < t) {
                        maxLoop = std::max(currentLoop, maxLoop);
                        currentLoop = 0;
                        if (i >= 16)
                            break;
                    } else {
                        currentLoop++;
                    }
                }
                if (currentLoop >= 12) {
                    img.at<uint8_t>((y), (x)*3+2) = 255;
                    img.at<uint8_t>((y+1), (x)*3+2) = 255;
                    img.at<uint8_t>((y-1), (x)*3+2) = 255;
                    img.at<uint8_t>((y), (x+1)*3+2) = 255;
                    img.at<uint8_t>((y), (x-1)*3+2) = 255;

                    img.at<uint8_t>((y), (x)*3+1) = 0;
                    img.at<uint8_t>((y+1), (x)*3+1) = 0;
                    img.at<uint8_t>((y-1), (x)*3+1) = 0;
                    img.at<uint8_t>((y), (x+1)*3+1) = 0;
                    img.at<uint8_t>((y), (x-1)*3+1) = 0;
                    frame.addMatch(NDVector<int, 2>{x, y});
                }
            }
        }
    }
    std::cout << std::endl;
    auto itPrev = images.begin();
    for (auto it = ++images.begin(); it != images.end(); itPrev++, it++) {

        Frame &framePrev = (itPrev)->second;
        Frame &frame = (it)->second;

        cv::Mat merged;
        cv::vconcat(framePrev._image, frame._image, merged);

        // std::unordered_map<uint128_t, Descriptor const *> descriptor;
        // std::unordered_map<uint128_t, Descriptor const *> descriptorPrev;

        // for (Descriptor const &des : frame._descriptors)
        //     descriptor.emplace(des._brief.res, &des);
        // for (Descriptor const &des : framePrev._descriptors)
        //     descriptorPrev.emplace(des._brief.res, &des);
        NDVector<float, 2> averageDep = {0, 0};
        float totDep = 0;
        for (Descriptor const &des : frame._descriptors) {
            int minDiff = INT_MAX;
            Descriptor const *best = 0;
            for (Descriptor const &desPrev : framePrev._descriptors) {
                int diff = des._brief.diff(desPrev._brief);
                if (diff < minDiff) {
                    minDiff = diff;
                    best = &desPrev;
                }
            }
            averageDep += (des-*best)/(minDiff+1);
            totDep += (minDiff+1);

            // std::cout << minDiff << std::endl;  
            if (minDiff > 10)
                continue;
            cv::Point p1((*best)[0], (*best)[1]), p2(des[0], des[1]+framePrev._size[1]);
            cv::line(merged, p1, p2, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
        }
        averageDep /= totDep;

        cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE);
        cv::imshow("Display Image", merged);
        cv::waitKey(0);
    }
    return 0;
}