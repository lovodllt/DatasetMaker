#include "ImageLabel.h"
#include "leftPart.h"
#include "detection.h"
#include "../../../../../usr/include/math.h"

ImageLabel::ImageLabel(QWidget *parent) : QLabel(parent)
{
    is_drawing = false;
    is_adjustingPoint = false;
    adjustPointIndex = -1;
    hoverPointIndex = -1;
    currentAdjustLabel = nullptr;

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
        circle(img, posePoint, 3, cv::Scalar(0, 255, 0), -1);
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

    double diagonal = sqrt(pow(img.cols, 2) + pow(img.rows, 2));
    minDistance = static_cast<int>(diagonal * 0.01);

    if (labelMode_ == "cls" || labelMode_ == "detection")
    {
        // 绘制已有标签
        if (!detectionLabels_.empty())
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

                    for (int i = 0; i < label.armor_points.size(); i++)
                    {
                        if (i == adjustPointIndex || i == hoverPointIndex)
                        {
                            circle(img, label.armor_points[i], minDistance, cv::Scalar(255, 255, 255), -1);
                            circle(img, label.armor_points[i], minDistance, cv::Scalar(255, 0, 0), ceil(minDistance / 4));
                        }
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
                    if (labelSave_)
                    {
                        putText(img, label.name, label.rect.tl(), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, 0.5, cv::LINE_AA);
                    }
                    if (colorSave_)
                    {
                        putText(img, label.color, cv::Point(label.rect.tl().x + 30, label.rect.tl().y), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, 0.5, cv::LINE_AA);
                    }
                }

                if (autoMode_ && label.confidence > confidence_threshold_)
                {
                    putText(img, QString::number(label.confidence, 'f', 2).toStdString(), cv::Point(label.rect.x + 90, label.rect.y), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, 0.5, cv::LINE_AA);
                }
            }
        }

        // 绘制临时标签
        if (is_labeling_)
        {
            if (is_poseMode_)
            {
                drawPose(img);
            }
            else
            {
                drawDetection(img);
            }
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
    if (labelMode_ == "cls")
    {
        leftPartInstance->clsInstance->displayPreview();
    }
    else if (labelMode_ == "detection" && labelSave_)
    {
        if (!tmpLabel.rect.empty())
        {
            cv::Mat roi = img(tmpLabel.rect);
            leftPartInstance->detectionInstance->displayPreview(roi);
        }
        else
        {
            for (auto label : detectionLabels_)
            {
                if (label.is_selected)
                {
                    leftPartInstance->detectionInstance->displayPreview(label.warp);
                    break;
                }
            }
        }
    }

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

    // 获取鼠标相对于视图的位置
    QPoint viewportMousePos = event->pos();

    // 计算原始图像坐标
    int originalX = static_cast<int>(viewportMousePos.x() / currentScale);
    int originalY = static_cast<int>(viewportMousePos.y() / currentScale);
    cv::Point clickPoint = cv::Point(originalX, originalY);

    // 进入点调整模式
    if (is_hoveringPoint)
    {
        if (is_poseMode_ && hoverPointIndex != -1)
        {
            for (auto &label : detectionLabels_)
            {
                if (label.is_selected)
                {
                    adjustPointIndex = hoverPointIndex;

                    if (adjustPointIndex != -1)
                    {
                        is_adjustingPoint = true;
                        currentAdjustLabel = &label;
                        hoverPointIndex = -1;
                        event->accept();
                        return;
                    }
                }
            }
        }
    }

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

                emit statusMessageUpdate("已选中标签");
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
    else if (is_poseMode_ && is_labeling_)
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
    else if (tmpLabel.rect.empty() && is_labeling_)
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

    is_hoveringPoint = false;

    if (!is_adjustingPoint && labelSelected &&!is_drawing)
    {
        for (auto &label : detectionLabels_)
        {
            if (label.is_selected && label.is_pose)
            {
                hoverPointIndex = getAdjustPointIndex(currentPoint, label);
                if (hoverPointIndex != -1)
                {
                    is_hoveringPoint = true;
                    break;
                }
            }
        }
        drawLabels();
    }

    if (is_adjustingPoint && currentAdjustLabel)
    {
        currentAdjustLabel->armor_points[adjustPointIndex] = currentPoint;

        // 重新计算边界框
        float xmin = FLT_MAX, xmax = FLT_MIN;
        float ymin = FLT_MAX, ymax = FLT_MIN;
        for (const auto &point : currentAdjustLabel->armor_points)
        {
            xmin = std::min(xmin, std::min(xmin, point.x));
            xmax = std::max(xmax, std::max(xmax, point.x));
            ymin = std::min(ymin, std::min(ymin, point.y));
            ymax = std::max(ymax, std::max(ymax, point.y));
        }
        currentAdjustLabel->rect = cv::Rect(xmin, ymin, xmax - xmin, ymax - ymin);
        currentAdjustLabel->center = getCenterFromPose(currentAdjustLabel->armor_points);

        drawLabels();
        event->accept();
        return;
    }

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

    if (is_adjustingPoint)
    {
        is_hoveringPoint = false;
        is_adjustingPoint = false;
        adjustPointIndex = -1;
        hoverPointIndex = -1;

        if (labelMode_ == "detection" && labelSave_ && is_warp_ && currentAdjustLabel)
        {
            cv::Mat img = getCurrentImage();

            if (!img.empty() && currentAdjustLabel->armor_points.size() == 4)
            {
                cv::Mat warpImg = leftPartInstance->autoModeInstance->warp(img, currentAdjustLabel->armor_points);
                if (!warpImg.empty())
                {
                    std::cout<<currentAdjustLabel->rect;
                    currentAdjustLabel->warp = warpImg;
                    if (is_binary_)
                    {
                        cvtColor(currentAdjustLabel->warp, currentAdjustLabel->warp, cv::COLOR_BGR2GRAY);
                        threshold(currentAdjustLabel->warp, currentAdjustLabel->warp, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
                        cvtColor(currentAdjustLabel->warp, currentAdjustLabel->warp, cv::COLOR_GRAY2BGR);
                    }
                }
                else
                {
                    currentAdjustLabel->warp = img(currentAdjustLabel->rect);
                }
            }
        }

        leftPartInstance->detectionInstance->updateLabelList();

        emit statusMessageUpdate("标签修改完成");
        event->accept();
        return;
    }

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
        setFocus();

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

    is_poseMode_ = false;

    is_hoveringPoint = false;
    is_adjustingPoint = false;
}

// 获取调整点索引
int ImageLabel::getAdjustPointIndex(const cv::Point& clickPoint, detectionLabel &label)
{
    if (!label.is_pose || label.armor_points.size() != 4)
    {
        return -1;
    }

    for (int i = 0; i < label.armor_points.size(); i++)
    {
        double dx = clickPoint.x - label.armor_points[i].x;
        double dy = clickPoint.y - label.armor_points[i].y;
        double distance = std::sqrt(dx * dx + dy * dy);

        if (distance < minDistance * 1.2)
        {
            return i;
        }
    }

    return -1;
}