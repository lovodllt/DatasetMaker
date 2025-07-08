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
    void drawLabels();
    void clearLabels();
    bool selectLabel(const cv::Point &point, const detectionLabel &label);

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
    cv::Mat preview;
    double currentScale;
    detectionLabel tmpLabel;
    bool labelSelected = false;
};

#endif // LABELPAINTER_H
