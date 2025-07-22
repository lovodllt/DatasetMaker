#include "ImageLabel.h"
#include "leftPart.h"
#include "detection.h"

ImageLabel::ImageLabel(QWidget *parent) : QLabel(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
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
    // 绘制临时标签
    if (!tmpLabel.rect.empty() && !is_poseMode_)
    {
        rectangle(img, tmpLabel.rect, cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
    }

    // 绘制鼠标框选
    if (is_drawing && firstPoint != cv::Point(0, 0))
    {
        rectangle(img, cv::Rect(firstPoint, currentPoint), cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
    }
}

void ImageLabel::drawPose(cv::Mat &img)
{
    // 绘制确定点
    for (auto &posePoint : posePoints)
    {
        circle(img, posePoint, 10, cv::Scalar(0, 255, 0), -1);
    }

    // 绘制已确定线段
    for (int i = 1; i < posePoints.size(); i++)
    {
        line(img, posePoints[i - 1], posePoints[i], cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
    }

    // 绘制动态线段
    if (!posePoints.empty() && currentPoint != cv::Point(0, 0))
    {
        line(img, posePoints.back(), currentPoint, cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
    }
}

void ImageLabel::drawLabels()
{
    cv::Mat img = getCurrentImage().clone();
    if (img.empty())
    {
        return;
    }

    originalImg = img.clone();

    if (labelMode_ == "cls" || labelMode_ == "detection")
    {
        // 绘制已有标签
        if (!detectionLabels_.empty() && is_labeling_)
        {
            for (const auto &label : detectionLabels_)
            {
                // 动态颜色和线宽
                cv::Scalar color = label.is_selected ? cv::Scalar(255, 0, 0) : cv::Scalar(0, 0, 255);
                int thickness = label.is_selected? 2 : 1;

                if (!label.is_pose)
                {
                    rectangle(img, label.rect, color, thickness, cv::LINE_AA);
                }
                else
                {
                    for (int i = 0; i < 4; i++)
                    {
                        line(img, label.armor_points[i], label.armor_points[(i + 1) % 4], color, thickness, cv::LINE_AA);
                    }
                }

                // 动态字体大小
                double fontScale = std::max(0.5, std::min(1.0, 0.005 * label.rect.area()));

                if (labelMode_ == "cls")
                {
                    putText(img, label.name, label.rect.tl(), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, 0.5, cv::LINE_AA);
                }
                else if (labelMode_ == "detection")
                {
                    if (colorSave_)
                    {
                        putText(img, label.color, label.rect.tl(), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, 0.5, cv::LINE_AA);
                    }
                }

                if (autoMode_ && label.confidence > confidence_threshold_)
                {
                    putText(img, QString::number(label.confidence, 'f', 2).toStdString(), cv::Point(label.rect.x + 60, label.rect.y), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, 0.5, cv::LINE_AA);
                }
            }
        }

        if (is_poseMode_)
        {
            drawPose(img);
        }
        else
        {
            drawDetection(img);
        }
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

    // 选中标签
    if (tmpLabel.rect.empty())
    {
        for (auto &label : detectionLabels_)
        {
            if (selectLabel(clickPoint, label))
            {
                firstPoint = cv::Point(0, 0);
                is_drawing = false;

                for (auto &label : detectionLabels_)
                {
                    label.is_selected = false;
                }

                label.is_selected = true;
                labelSelected = true;

                emit onLabelSelected(label);

                QString message = tr("标签模式: 选中标签 '%1'").arg(QString::fromStdString(label.name));
                emit statusMessageUpdate(message);
                break;
            }

            labelSelected = false;
        }
    }

    // 选中标签时更新图像
    if (labelSelected)
    {
        drawLabels();
    }
    // 未选中且pose模式开始绘制点
    else if (is_poseMode_)
    {
        if (!posePoints.empty())
        {
            is_drawing = false;

            double dx = posePoints[posePoints.size() - 1].x - clickPoint.x;
            double dy = posePoints[posePoints.size() - 1].y - clickPoint.y;
            double distance = std::sqrt(dx * dx + dy * dy);

            if (distance < 20)
            {
                emit statusMessageUpdate("定点失败: 距离过近");
                return;
            }
        }

        posePoints.push_back(clickPoint);

        QString message = tr("已标记 %1 个点").arg(posePoints.size());
        emit statusMessageUpdate(message);

        if (posePoints.size() == 4)
        {
            leftPartInstance->detectionInstance->makePoseLabel(posePoints);

            posePoints.clear();
        }

        drawLabels();
    }
    // 未选中时开始绘制临时标签
    else if (tmpLabel.rect.empty())
    {
        firstPoint = clickPoint;
        is_drawing = true;

        emit statusMessageUpdate("标签模式: 松开鼠标确定临时标签");
    }
    else
    {
        QLabel::mousePressEvent(event);
    }
}

// 移动事件
void ImageLabel::mouseMoveEvent(QMouseEvent *event)
{
    QPoint viewportMousePos = event->pos();
    int originalX = static_cast<int>(viewportMousePos.x() / currentScale);
    int originalY = static_cast<int>(viewportMousePos.y() / currentScale);
    currentPoint = cv::Point(originalX, originalY);

    if (is_drawing || !posePoints.empty())
    {
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
    QPoint viewportMousePos = event->pos();

    // 计算原始图像坐标
    int originalX = static_cast<int>(viewportMousePos.x() / currentScale);
    int originalY = static_cast<int>(viewportMousePos.y() / currentScale);
    cv::Point lastPoint = cv::Point(originalX, originalY);

    if (event->button() == Qt::LeftButton && is_drawing && firstPoint != cv::Point(0, 0) && tmpLabel.rect.empty())
    {
        // 记录标签
        tmpLabel.rect = cv::Rect(firstPoint, lastPoint);

        if (tmpLabel.rect.area() < 100)
        {
            tmpLabel.rect = cv::Rect();
            emit statusMessageUpdate("标签创建失败: 标签面积太小");
        }

        firstPoint = cv::Point(0, 0);
        is_drawing = false;

        for (auto &label : detectionLabels_)
        {
            label.is_selected = false;
        }

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
    if (event->key() == Qt::Key_Escape && (is_drawing || !posePoints.empty()))
    {
        firstPoint = cv::Point(0, 0);
        tmpLabel = detectionLabel();
        posePoints.clear();
        drawLabels();
    }
    else if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        if (labelSelected)
        {
            auto index = std::remove_if(detectionLabels_.begin(), detectionLabels_.end(), [](const detectionLabel &label) {
                return label.is_selected;
            });

            if (index != detectionLabels_.end())
            {
                detectionLabels_.erase(index, detectionLabels_.end());
                drawLabels();
            }

            if (labelMode_ == "detection")
            {
                leftPartInstance->detectionInstance->updateLabelList();
            }

            labelSelected = false;
        }
        else if (!tmpLabel.rect.empty())
        {
            tmpLabel = detectionLabel();
            drawLabels();
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
    tmpLabel = detectionLabel();
    posePoints.clear();

    labelSelected = false;

    for (auto &label : detectionLabels_)
    {
        label.is_selected = false;
    }
}