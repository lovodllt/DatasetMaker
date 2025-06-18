#ifndef LEFTPART_H
#define LEFTPART_H

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
#include <QDebug>
#include <QScrollBar>
#include <QDir>
#include <QFileInfo>

namespace Ui { class leftPart; }

class leftPart : public QWidget
{
    Q_OBJECT

public:
    explicit leftPart(QWidget *parent = nullptr);
    ~leftPart();

    void displayImage(const QString &imagePath);
    void populateImageList(const QFileInfoList &imageFiles, QMap<QString, bool> &is_images_processed);

signals:
    void buttonClicked(int index);

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
    void resizeEvent(QResizeEvent *evrnt) override;

private:
    void updateImageDisplay();

    Ui::leftPart *ui;

    QScrollArea *scrollArea;                   // 滚动区域
    QLabel *imageLabel;                        // 图像显示
    QListWidget *imageListWidget;              // 图像文件列表
    QFileInfoList imageFiles;                  // 存储图片信息
    QMap<QString, bool> is_images_processed;
    QPixmap currentPixmap;                     // 存储当前显示的图像
    double scaleFactor = 1.0;                  // 缩放因子
    QSize originalImageSize;                   // 原始图像尺寸
    bool isZoomed = false;                     // 是否进行了缩放

};

#endif // LEFTPART_H
