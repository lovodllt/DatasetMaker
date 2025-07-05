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

struct detectionLabel
{
    std::string name;
    cv::Rect rect;
    bool is_selected = false;
    bool is_saved = false;
};

extern QString savePath_;
extern QString saveFormat_;
extern QString labelMode_;
extern bool is_labeling_;
extern double confidence_;
extern std::vector<detectionLabel> detectionLabels_;
extern bool is_autoCut_;
extern int saveId_;

#endif //COMMON_H
