#ifndef LEFTPART_H
#define LEFTPART_H
#pragma once

#include <QListWidgetItem>
#include <QMap>
#include <QSize>
#include <QPixmap>
#include <QScrollArea>
#include <QScrollBar>
#include <QFileDialog>
#include "common.h"
#include "ImageLabel.h"

namespace Ui { class leftPart; }

const double zoomFactor = 1.1;

class leftPart : public QWidget
{
    Q_OBJECT

public:
    explicit leftPart(QWidget *parent = nullptr);
    ~leftPart();

    void openFile();
    void processDirectory(const QString& dirPath);
    void displayImage(const QString &imagePath);
    void populateImageList(const QFileInfoList &imageFiles, QMap<QString, bool> &is_images_processed);
    void saveFilePath();

signals:
    void statusMessageUpdate(const QString &message);

private slots:
    void on_openDir_clicked();
    void on_nextImage_clicked();
    void on_prevImage_clicked();
    void on_createLabel_clicked();
    void on_save_clicked();
    void on_deleteFile_clicked();

    void on_imageListWidget_itemClicked(QListWidgetItem *item);

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    void setCurrentImage(int index);

    Ui::leftPart *ui;

    QScrollArea *imageArea;                    // 滚动区域
    QFileInfoList imageFiles;                  // 存储图片信息
    QMap<QString, bool> is_images_processed;   // 是否处理了图像
    int currentIndex = -1;                     // 当前显示的图片索引

public:
    ImageLabel *imageLabel;

    QPixmap originalPixmap;                    // 存储当前显示的原始图像
    QSize originalImageSize;                   // 原始图像尺寸
    double currentScale;
};

#endif // LEFTPART_H
