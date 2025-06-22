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
    void setLabels(const QVector<QRect> newLabels);
    void setDrawingState(bool drawing);

    void clearFirstPoint();
    void clearPreview();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QSize originalImageSize;
    QVector<QRect> labels;
    QRect previewRect;
    QPoint firstPoint;
    bool isDrawing = false;
};

#endif // LABELPAINTER_H
