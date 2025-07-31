#ifndef LABELPAINTER_H
#define LABELPAINTER_H
#pragma once

#include <QLabel>
#include <QMouseEvent>
#include "common.h"

class leftPart;

class ImageLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ImageLabel(QWidget *parent = nullptr);
    cv::Mat getCurrentImage();
    void drawDetection(cv::Mat &img);
    void drawPose(cv::Mat &img);
    void drawLabels();
    void clearLabels();
    bool selectLabel(const cv::Point &point, const detectionLabel &label);
    int getAdjustPointIndex(const cv::Point& clickPoint, detectionLabel &label);

signals:
    void statusMessageUpdate(const QString &message);
    void previewRequested();
    void onLabelSelected(detectionLabel label);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    bool is_drawing;
    cv::Point firstPoint;
    cv::Point currentPoint;

public:
    leftPart *leftPartInstance;

    cv::Mat originalImg;
    double currentScale;
    detectionLabel tmpLabel;
    std::vector<cv::Point> posePoints;

    bool is_hoveringPoint;                  // 是否悬停在某点上
    bool is_adjustingPoint;                 // 是否正在调整点的位置
    int minDistance = 20;
    int adjustPointIndex;                   // 当前调整的点的索引
    int hoverPointIndex;                    // 当前悬停的点的索引
    detectionLabel *currentAdjustLabel;     // 当前调整的标签的指针
};

#endif // LABELPAINTER_H
