#include "ImageLabel.h"
#include "ImageLabel.h"
#include "leftPart.h"

ImageLabel::ImageLabel(QWidget *parent) : QLabel(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

cv::Mat ImageLabel::getCurrentImage()
{
    if (pixmap()->isNull())
    {
        return cv::Mat();
    }

    // 统一转换为RGB888格式并转换为cv::Mat
    QImage qImg = leftPartInstance->originalPixmap.toImage().convertToFormat(QImage::Format_RGB888);
    return cv::Mat(qImg.height(), qImg.width(), CV_8UC3, (void*)qImg.constBits(), qImg.bytesPerLine()).clone();
}

void ImageLabel::drawDetection(cv::Mat &img)
{
    // 绘制已有标签
    if (!detectionLabels_.empty())
    {
        for (const auto &label : detectionLabels_)
        {
            cv::Scalar color = label.is_selected ? cv::Scalar(255, 0, 0) : cv::Scalar(0, 0, 255);
            int thickness = label.is_selected? 2 : 1;

            rectangle(img, label.rect, color, thickness, cv::LINE_AA);
            putText(img, label.name, label.rect.tl(), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 0.3, cv::LINE_AA);

            if (labelMode_ == "cls" && label.is_selected)
            {
                preview = img(label.rect);
            }
        }
    }

    // 绘制临时标签
    if (!tmpLabel.rect.empty())
    {
        rectangle(img, tmpLabel.rect, cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
    }

    // 绘制鼠标框选
    if (is_drawing && firstPoint != cv::Point(0, 0))
    {
        rectangle(img, cv::Rect(firstPoint, currentPoint), cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
    }
}

void ImageLabel::drawLabels()
{
    cv::Mat img = getCurrentImage().clone();
    if (img.empty() || !is_labeling_)
    {
        return;
    }

    originalImg = img.clone();

    if (labelMode_ == "cls" || labelMode_ == "detection")
    {
        drawDetection(img);
    }

    cv::Mat displayImg;
    if (img.channels() == 3)
    {
        displayImg = img.clone();
    }
    else if (img.channels() == 4)
    {
        cvtColor(img, displayImg, cv::COLOR_RGBA2RGB);
    }

    // 转换为Qt图像（直接使用RGB888格式）
    QImage qImg(img.data, img.cols, img.rows, img.step, QImage::Format_RGB888);

    QSize scaledSize = qImg.size() * currentScale;
    QPixmap scaledPixmap = QPixmap::fromImage(qImg).scaled(
        scaledSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    setPixmap(scaledPixmap);
    resize(scaledSize);

    // 预览标签
    emit previewRequested();

    update();
}

bool ImageLabel::selectLabel(const cv::Point &point, const detectionLabel &label)
{
    return label.rect.contains(point);
}

// 点击事件
void ImageLabel::mousePressEvent(QMouseEvent *event)
{
    if (labelMode_.isNull())
    {
        QString message = tr("标签模式: 请先选择标签模式");
        emit statusMessageUpdate(message);
        return;
    }

    if (!is_labeling_)
        return;

    // 获取鼠标相对于视图的位置
    QPoint viewportMousePos = event->pos();

    // 计算原始图像坐标
    int originalX = static_cast<int>(viewportMousePos.x() / currentScale);
    int originalY = static_cast<int>(viewportMousePos.y() / currentScale);
    cv::Point clickPoint = cv::Point(originalX, originalY);

    for (auto &label : detectionLabels_)
    {
        if (selectLabel(clickPoint, label))
        {
            firstPoint = cv::Point(0, 0);
            is_drawing = false;

            for (auto &otherLabel : detectionLabels_)
            {
                otherLabel.is_selected = false;
            }

            label.is_selected = true;
            labelSelected = true;

            QString message = tr("标签模式: 选中标签 '%1'").arg(QString::fromStdString(label.name));
            emit statusMessageUpdate(message);
            break;
        }
    }

    if (labelSelected)
    {
        drawLabels();
    }

    if (event->button() == Qt::LeftButton && firstPoint == cv::Point(0, 0) && tmpLabel.rect.empty())
    {
        firstPoint = clickPoint;
        is_drawing = true;

        QString message = tr("标签模式: 松开鼠标确定标签");
        emit statusMessageUpdate(message);
    }
    else
    {
        QLabel::mousePressEvent(event);
    }
}

// 移动事件
void ImageLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (is_drawing)
    {
        QPoint viewportMousePos = event->pos();
        int originalX = static_cast<int>(viewportMousePos.x() / currentScale);
        int originalY = static_cast<int>(viewportMousePos.y() / currentScale);
        currentPoint = cv::Point(originalX, originalY);

        drawLabels();
    }
    else
    {
        QLabel::mouseMoveEvent(event);
    }
}

// 释放事件
void ImageLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && is_drawing && firstPoint != cv::Point(0, 0) && tmpLabel.rect.empty())
    {
        QPoint viewportMousePos = event->pos();

        // 计算原始图像坐标
        int originalX = static_cast<int>(viewportMousePos.x() / currentScale);
        int originalY = static_cast<int>(viewportMousePos.y() / currentScale);
        cv::Point lastPoint = cv::Point(originalX, originalY);

        // 记录标签
        tmpLabel.rect = cv::Rect(firstPoint, lastPoint);

        firstPoint = cv::Point(0, 0);
        is_drawing = false;

        drawLabels();

        QString message = tr("标签模式: 标签创建成功");
        emit statusMessageUpdate(message);
    }
    else
    {
        QLabel::mouseReleaseEvent(event);
    }
}

// 键盘事件重写
void ImageLabel::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && is_drawing)
    {
        clearLabels();
        drawLabels();
    }
    else if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace && labelSelected)
    {
        for (auto it = detectionLabels_.begin(); it != detectionLabels_.end();)
        {
            if (it->is_selected)
            {
                it = detectionLabels_.erase(it);
                drawLabels();
            }
            else
            {
                ++it;
            }
        }
    }
    else
    {
        QLabel::keyPressEvent(event);
    }
}

// 清空标签
void ImageLabel::clearLabels()
{
    firstPoint = cv::Point(0, 0);
    currentPoint = cv::Point(0, 0);
    tmpLabel = detectionLabel();

    for (auto &label : detectionLabels_)
    {
        label.is_selected = false;
    }
}