#ifndef LEFTPART_H
#define LEFTPART_H

#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QFileInfoList>
#include <QListWidgetItem>
#include <QMap>
#include <QSize>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QPixmap>
#include <QScrollArea>
#include <QDebug>
#include <QScrollBar>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QKeyEvent>

#include "ImageLabel.h"

namespace Ui { class leftPart; }

extern QString savePath_;
extern QString saveFormat_;
extern QString labelMode_;

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
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void updateImageDisplay(double customScale = -1);
    void setCurrentImage(int index);
    QPoint convertToImageCoordinates(const QPoint &pos);
    void cancelDrawing();
    QRect calculateRect(const QPoint &start, const QPoint &end);

    Ui::leftPart *ui;

    ImageLabel *imageLabel;

    QScrollArea *imageArea;                    // 滚动区域
    QFileInfoList imageFiles;                  // 存储图片信息
    QMap<QString, bool> is_images_processed;   // 是否处理了图像
    QPixmap currentPixmap;                     // 存储当前显示的图像
    QSize originalImageSize;                   // 原始图像尺寸
    int currentIndex = -1;                     // 当前显示的图片索引

    bool is_labeling = false;                  // 是否处于标签模式
    QPoint firstPoint;                         // 矩形的第一个点
    QPoint currentPoint;                       // 鼠标当前指向的点
    bool isDrawingRect = false;                // 是否正在绘制矩形
};

#endif // LEFTPART_H
