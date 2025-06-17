#ifndef LEFT_H
#define LEFT_H

#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QFileInfoList>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMap>
#include <QSize>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QPixmap>
#include <QPoint>
#include <QScrollArea>

namespace Ui { class left; }

class left : public QWidget
{
    Q_OBJECT

public:
    explicit left(QWidget *parent = nullptr);
    ~left();

    void displayImage(const QString &imagePath);
    void populateImageList(const QFileInfoList &imageFiles, QMap<QString, bool> &is_images_processed);

signals:
    void buttonClicked(int index);

private slots:
    void on_openDir_clicked();
    void on_nextImage_clicked();
    void on_prev_Image_clicked();
    void on_imageListWidget_itemClicked(QListWidgetItem *item);
    void onWindowResized(const QSize &size);

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    Ui::left *ui;

    QScrollArea *scrollArea;                   // 滚动区域
    QLabel *imageLabel;                        // 图像显示
    QListWidget *imageListWidget;              // 图像文件列表
    QFileInfoList imageFiles;                  // 存储图片信息
    QMap<QString, bool> is_images_processed;
    QPixmap currentPixmap;                     // 存储当前显示的图像
    double scaleFactor = 1.0;                  // 缩放因子
    QPointF scrollOffset;                      // 滚动偏移量
    QSize originalImageSize;                   // 原始图像尺寸
    QSize lastDisplaySize;                     // 上一时刻的尺寸
    bool isZoomed = false;                     // 是否进行了缩放
};
#endif // LEFT_H
