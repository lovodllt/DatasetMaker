#include "ImageLabel.h"

ImageLabel::ImageLabel(QWidget *parent) : QLabel(parent) {}

void ImageLabel::setOriginalImageSize(const QSize &size)
{
    originalImageSize = size;
}

void ImageLabel::setFirstPoint(const QPoint &point)
{
    firstPoint = point;
    update();
}

void ImageLabel::setPreviewRect(const QRect &rect)
{
    previewRect = rect;
    update();
}

void ImageLabel::setLabels(const QVector<QRect> newLabels)
{
    labels = newLabels;
    update();
}

void ImageLabel::clearFirstPoint()
{
    firstPoint = QPoint();
    update();
}

void ImageLabel::clearPreview()
{
    previewRect = QRect();
    update();
}

void ImageLabel::setDrawingState(bool drawing)
{
    isDrawing = drawing;
    update();
}

void ImageLabel::paintEvent(QPaintEvent *event)
{
    // 确保默认绘制行为
    QLabel::paintEvent(event);

    if (pixmap() && !pixmap()->isNull())
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);    // 抗锯齿

        QSize pixmapSize = pixmap()->size();

        double scaleX = static_cast<double>(pixmapSize.width()) / originalImageSize.width();
        double scaleY = static_cast<double>(pixmapSize.height()) / originalImageSize.height();

        // 绘制现有标签
        if (!labels.isEmpty())
        {
            QPen pen(Qt::green, 2);
            painter.setPen(pen);

            for (const QRect &rect : labels)
            {
                QRect displayRect(
                    static_cast<int>(rect.x() * scaleX),
                    static_cast<int>(rect.y() * scaleY),
                    static_cast<int>(rect.width() * scaleX),
                    static_cast<int>(rect.height() * scaleY)
                );

                painter.drawRect(displayRect);
            }
        }

        // 绘制第一个点
        if (!firstPoint.isNull())
        {
            QPoint displayPoint(
                static_cast<int>(firstPoint.x() * scaleX),
                static_cast<int>(firstPoint.y() * scaleY)
            );

            // 确保点在可见区域内
            displayPoint.setX(qBound(0, displayPoint.x(), pixmapSize.width() - 1));
            displayPoint.setY(qBound(0, displayPoint.y(), pixmapSize.height() - 1));

            painter.setBrush(Qt::green);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(displayPoint, 4, 4); // 4像素半径的圆点
            painter.setBrush(Qt::NoBrush);
        }

        // 绘制预览矩形
        if (previewRect.isValid())
        {
            QRect displayRect(
                   static_cast<int>(previewRect.x() * scaleX),
                   static_cast<int>(previewRect.y() * scaleY),
                   static_cast<int>(previewRect.width() * scaleX),
                   static_cast<int>(previewRect.height() * scaleY)
            );

            // 根据绘制状态选择不同样式
            QColor fillColor(isDrawing ? QColor(128, 128, 255, 80) : QColor(128, 128, 128, 40));
            QColor borderColor(isDrawing ? Qt::green : Qt::gray);

            painter.setPen(QPen(borderColor, 2, Qt::DashLine));
            painter.setBrush(fillColor);
            painter.drawRect(displayRect);
            painter.setBrush(Qt::NoBrush);
        }
    }
}


