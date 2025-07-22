#ifndef COMMON_H
#define COMMON_H
#pragma once

#include <memory>
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
    cv::Point2f center;
    double confidence;
    std::vector<cv::Point2f> armor_points;
    cv::Mat warp;
    std::string color;
    bool is_selected = false;
    bool is_saved = false;
    bool is_pose = false;
};

struct finalArmor
{
    cv::Rect box;
    double confidence;
    std::string color;
    cv::Point2f center;
    std::vector<cv::Point2f> armor_points;
    cv::Mat warp;
    cv::Mat num;
    std::string label;
};

// common
extern QString savePath_;
extern QString saveFormat_;
extern QString labelMode_;
extern QMap<QString, bool> is_images_processed;
extern bool is_labeling_;
extern std::vector<detectionLabel> detectionLabels_;
extern int saveId_;
extern bool autoMode_;
// cls
extern bool is_autoCut_;
extern bool is_warp_;
extern bool is_binary_;
// autoMode
extern double confidence_threshold_;
extern double nms_threshold_;
extern std::vector<finalArmor> finalArmors_;
extern std::string modelSelection_;
extern const std::string v8_model_path_;
extern const std::string v12_model_path_;
extern const std::string cls_model_path_;
// detection
extern bool colorSave_;
extern bool labelSave_;
extern bool is_poseMode_;

extern std::string getModelPath(const std::string modelPath);
extern cv::Point2f getCenterFromPose(std::vector<cv::Point2f> armor_points);


#endif //COMMON_H
