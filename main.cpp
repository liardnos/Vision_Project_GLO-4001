#include <stdio.h>
#include <filesystem>
#include <map>

#include "main.hpp"


template <class T, class G, uint D>
std::vector<T *> findNClose(UniTree<T, G, D> &tree, G pos, int n) {
    G size;
    for (uint i = 0; i < D; i++)
        size[i] = 1;
    std::vector<T *> res;
    // tree.draw([](NDVector<float, 2> &a, NDVector<float, 2> &b, uint d) {
    //     std::cout << d << std::endl;
    // });
    while (res.size() < n) {
        res = tree.getInArea(pos, size);
        size *= 2;
        // std::cout << DEBUGVAR(pos) << std::endl;
        // std::cout << DEBUGVAR(size) << std::endl;
        // std::cout << DEBUGVAR(res.size()) << std::endl;
    }
    std::sort(res.begin(), res.end(), 
        [&pos](const T * a, const T * b) {
            return (*a-pos).lengthSquare() < (*b-pos).lengthSquare();
        }
    );
    return res;
}

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

Descriptor::Descriptor(cv::Mat const &img, NDVector<float, 2> const &p) : 
    NDVector<float, 2>(p),
    _brief(img, p.round().cast<int>())
{}

Frame::Frame(std::string const &path) : 
    _image(cv::imread(path, 1)),
    _size(NDVector<int, 2>{_image.size().width, _image.size().height})
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
        { 3, -1}, { 3,  0}, { 3,  1}, 
        { 2,  2},
        { 1,  3}, { 0,  3}, {-1,  3},
        {-2,  2},
        {-3,  1}, {-3,  0}, {-3, -1},
        {-2, -2},
        {-1, -3}, { 0, -3}, { 1, -3},
        { 2, -2},
    };

    NDVector<int, 2> fastSize = {32, 32};
    if (!"correct for sqrt") {
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
        frame._fastMask = cv::Mat(frame._image.size().height, frame._image.size().width, CV_8UC3, cv::Scalar(0, 0, 0));
        std::cout << "|" << std::flush;
        NDVector<int, 2> imgSize = {img.size().width, img.size().height};
        int const t = 20;

        for (int y = fastSize[1]; y < imgSize[1]-fastSize[1]; y++) {
            for (int x = fastSize[0]; x < imgSize[0]-fastSize[0]; x++) {
                int const p = img.at<uint8_t>(y, x*3);
                int maxLoop = 0;
                int currentLoop = 0;
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
                    // img.at<uint8_t>((y), (x)*3+2) = 255;
                    // img.at<uint8_t>((y+1), (x)*3+2) = 255;
                    // img.at<uint8_t>((y-1), (x)*3+2) = 255;
                    // img.at<uint8_t>((y), (x+1)*3+2) = 255;
                    // img.at<uint8_t>((y), (x-1)*3+2) = 255;

                    // img.at<uint8_t>((y), (x)*3+1) = 0;
                    // img.at<uint8_t>((y+1), (x)*3+1) = 0;
                    // img.at<uint8_t>((y-1), (x)*3+1) = 0;
                    // img.at<uint8_t>((y), (x+1)*3+1) = 0;
                    // img.at<uint8_t>((y), (x-1)*3+1) = 0;
                    // frame.addMatch(NDVector<int, 2>{x, y});
                    frame._fastMask.at<uint8_t>(y, x*3) = 255;
                }
            }
        }
        { // center of mass of features
            int dilation_type = cv::MORPH_ELLIPSE;
            int dilation_size = 1;
            cv::Mat element = cv::getStructuringElement(dilation_type, cv::Size(2*dilation_size+1, 2*dilation_size+1), cv::Point(dilation_size, dilation_size));
            cv::dilate(frame._fastMask, frame._fastMask, element);
            
            // declare Mat variables, thr, gray and src
            cv::Mat thr, gray;
            
            // convert image to grayscale
            cv::cvtColor(frame._fastMask, gray, cv::COLOR_BGR2GRAY);
            
            // convert grayscale to binary image
            // cv::threshold(gray, thr, 100, 255, cv::THRESH_BINARY);
            
            cv::Mat canny_output;
            std::vector<std::vector<cv::Point> > contours;
            std::vector<cv::Vec4i> hierarchy;
            
            // detect edges using canny
            cv::Canny(gray, canny_output, 50, 150, 3);
            
            // find contours
            cv::findContours(canny_output, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
            
            // get the moments
            std::vector<cv::Moments> mu(contours.size());
            for (uint i = 0; i < contours.size(); i++)
                mu[i] = moments(contours[i], false);
            
            // get the centroid of figures.
            std::vector<cv::Point2f> mc(contours.size());
            for (uint i = 0; i < contours.size(); i++)
                mc[i] = cv::Point2f(mu[i].m10/mu[i].m00 , mu[i].m01/mu[i].m00);
            
            // draw contours
            cv::Mat drawing(canny_output.size(), CV_8UC3, cv::Scalar(255,255,255));
            // memset(&frame._fastMask.at<uint8_t>(0, 0), 0, frame._fastMask.size().height*frame._fastMask.size().width*3);
            for (uint i = 0; i<contours.size(); i++) {
                cv::Scalar color(167,151,0); // B G R values
                cv::drawContours(drawing, contours, i, color, 2, 8, hierarchy, 0, cv::Point());
                if (!std::isnormal(mc[i].x))
                    continue;
                frame.addMatch(NDVector<float, 2>{mc[i].x, mc[i].y});

                int x = mc[i].x;
                int y = mc[i].y;
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

                cv::circle(drawing, mc[i], 4, color, -1, 8, 0);
            }
            
            // show the resultant image
            // cv::namedWindow("Contours", cv::WINDOW_AUTOSIZE);
            // cv::imshow("Contours", drawing);
            // cv::waitKey(0);
        }
        frame.updateTree();
    }
    std::cout << std::endl;
    auto itPrev = images.begin();
    for (auto it = ++images.begin(); it != images.end(); itPrev++, it++) {

        Frame &framePrev = (itPrev)->second;
        Frame &frame = (it)->second;

        cv::Mat merged;
        cv::vconcat(framePrev._image, frame._image, merged);
        cv::Mat fastMask(frame._image.size().height, frame._image.size().width, CV_8UC3, cv::Scalar(0, 0, 0));


        std::cout << DEBUGVAR(framePrev._descriptors.size()) << std::endl;  
        // std::unordered_map<uint128_t, Descriptor const *> descriptor;
        // std::unordered_map<uint128_t, Descriptor const *> descriptorPrev;

        // for (Descriptor const &des : frame._descriptors)
        //     descriptor.emplace(des._brief.res, &des);
        // for (Descriptor const &des : framePrev._descriptors)
        //     descriptorPrev.emplace(des._brief.res, &des);
        NDVector<float, 2> averageDep = {0, 0};
        float totDep = 0;
        for (Descriptor const &des : frame._descriptors) {
            fastMask.at<uint8_t>(des[1], des[0]*3+1) = 255;

            std::vector<Descriptor *> resprev = findNClose(*framePrev._fastMatch, NDVector<float, 2>{des}, 1);
            std::vector<Descriptor *> res = findNClose(*frame._fastMatch, NDVector<float, 2>{*resprev[0]}, 1);
            if (res[0] != &des)
                continue;

            int diff = des._brief.diff(resprev[0]->_brief);
            if (diff < 128*0.50) {
                cv::Point p1((*resprev[0])[0], (*resprev[0])[1]), p2(des[0], des[1]+framePrev._size[1]);
                cv::line(merged, p1, p2, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            }

        }
        averageDep /= totDep;



        // std::cout << type2str(merged.type()) << std::endl;
        cv::vconcat(merged, fastMask, merged);

        cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE);
        cv::imshow("Display Image", merged);
        // cv::waitKey(0);
        cv::waitKey(1E3/10);
    }
    return 0;
}