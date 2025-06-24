#ifndef LABELPAINTER_H
#define LABELPAINTER_H

#include <QLabel>
#include <QPainter>
#include <QVector>
#include <QRect>
#include <QPoint>

class ImageLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ImageLabel(QWidget *parent = nullptr);

    void setOriginalImageSize(const QSize &size);
    void setFirstPoint(const QPoint &point);
    void setPreviewRect(const QRect &rect);
    void setLabels(const QVector<QRect> &newLabels);
    double getCurrentScaleFactor() const { return currentScaleFactor; }
    QVector<QRect> getOriginalLabels() const { return originalLabels; }
    void setLabelsScaleFactor(double scale);
    void setDrawingState(bool drawing);

    void clearFirstPoint();
    void clearPreview();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QSize originalImageSize;
    QPoint firstPoint;
    QRect previewRect;
    double currentScaleFactor = 1.0;
    bool isDrawing = false;
    QVector<QRect> originalLabels;      // 原始图像的坐标标签
};

#endif // LABELPAINTER_H
