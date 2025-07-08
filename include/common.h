#ifndef COMMON_H
#define COMMON_H
#pragma once

#include <iostream>
#include <QString>
#include <vector>
#include <opencv2/opencv.hpp>
#include <QMap>
#include <QDirIterator>
#include <QMessageBox>
#include <QDebug>

struct detectionLabel
{
    std::string name;
    cv::Rect rect;
    cv::Mat warp;
    bool is_selected = false;
    bool is_saved = false;
};

struct finalArmor
{
    cv::Rect box;
    double confidence;
    std::string color;
    std::vector<cv::Point2f> armor_points;
    cv::Mat warp;
    cv::Mat num;
    std::string label;
};

extern QString savePath_;
extern QString saveFormat_;
extern QString labelMode_;
extern bool is_labeling_;
extern double confidence_threshold_;
extern double nms_threshold_;
extern std::vector<detectionLabel> detectionLabels_;
extern std::vector<finalArmor> finalArmors_;
extern bool is_autoCut_;
extern int saveId_;
extern bool autoMode_;
extern std::string modelSelection_;
extern bool is_warp_;
extern const std::string v8_model_path_;
extern const std::string v12_model_path_;
extern const std::string cls_model_path_;

#endif //COMMON_H
