#ifndef AUTOMODE_H
#define AUTOMODE_H
#pragma once
#include "common.h"

struct dataImg
{
    cv::Mat input;
    float scale;
    int padW;
    int padH;
    cv::Mat blob;
};

struct preArmor
{
    std::vector<cv::Rect> boxes;
    std::vector<float> confidence;
};

struct Bar
{
    cv::RotatedRect rect;
    cv::Point2f center;
    float ratio;
    float angle;
    std::vector<cv::Point2f> sorted_points;
    double long_one;
};

struct inferArmor
{
    cv::Rect box;
    double confidence;
    std::string color;
    cv::Mat original_roi;
    cv::Mat binary_roi;
    std::vector<Bar> bars;
};

static const std::vector<std::string> class_names_ = {
    "1", "2", "3", "4", "5", "outpost", "guard", "base", "negative"};

class autoMode : public QObject
{
    Q_OBJECT

public:
    explicit autoMode(QObject *parent = nullptr)
    {
        using_once();
    }
    ~autoMode() = default;

    void using_once();
    void init();

    void setModelPath(const std::string &detectio_path);
    dataImg preprocess(cv::Mat &img);
    void Inference(cv::Mat &img);
    void colorFiliter(cv::Mat &img, inferArmor &inferArmor);
    bool isValidBar(Bar &bar);
    void barFiliter();
    bool isValidArmor(inferArmor &armor);
    void armorFiliter();
    void noWarpFiliter();

    cv::Mat extractNumber(cv::Mat &img, finalArmor &armor);
    void classify(cv::Mat img);

    void update(cv::Mat &img);

signals:
    void statusMessageUpdate(const QString &message);

private:
    std::once_flag flag_;
    cv::dnn::Net detectioNet;
    cv::dnn::Net clsNet;
    int target_size = 640;
    float gamma = 0.55;
    std::vector<inferArmor> qualifiedArmors;
    bool is_cut = false;
    bool tmp_warp_close = false;
};

// HSV
#define red_h_max_low_       25
#define red_h_min_low_       0
#define red_h_max_high_      180
#define red_h_min_high_      140
#define red_s_max_           255
#define red_s_min_           40
#define red_v_max_           255
#define red_v_min_           140
#define blue_h_max_          130
#define blue_h_min_          80
#define blue_s_max_          255
#define blue_s_min_          30
#define blue_v_max_          255
#define blue_v_min_          190

// BAR
#define min_lw_ratio_        2
#define max_lw_ratio_        10
#define min_bar_angle_       55

// ARMOR
#define max_bars_ratio_      1.5
#define min_bars_distance_   0.4
#define max_bars_distance_   2
#define max_bars_angle_      45

// NUMBER
#define roi_w_ 20
#define roi_h_ 28
#define light_len_    12    // 灯条长度
#define warp_height_  28    // 透视变换后图像高度
#define warp_width_   32    // 透视变换后图像宽度(即小装甲板宽度)

#endif //AUTOMODE_H
