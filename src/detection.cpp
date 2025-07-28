#include "detection.h"

#include "ImageLabel.h"
#include "ui_detection.h"

detection::detection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::detection),
    leftPartInstance(nullptr)
{
    ui->setupUi(this);
    is_warp_ = false;
    is_binary_ = false;
    ui->widthEdit->setEnabled(false);
    ui->heightEdit->setEnabled(false);
    ui->binary->setEnabled(false);
    ui->warp->setEnabled(false);
    ui->labelSelection->setEnabled(false);

    colorSelection = ui->colorSelection;
    setFocusPolicy(Qt::StrongFocus);

    connect(colorSelection, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &detection::onColorSelected);
    connect(ui->labelList, &QListWidget::itemClicked, this, &detection::on_labelList_itemClicked);
}

detection::~detection()
{
    delete ui;
}

// 获取左侧功能栏指针
void detection::setLeftPart(leftPart *leftPart)
{
    this->leftPartInstance = leftPart;
    leftPartInstance->detectionInstance = this;
}

// 自动模式
void detection::on_autoMode_toggled(bool checked)
{
    autoMode_ = checked;

    if (autoMode_)
    {
        emit statusMessageUpdate("自动标注模式已开启");
        leftPartInstance->displayImage(leftPartInstance->getCurrentImagePath());
    }
    else
    {
        emit statusMessageUpdate("自动标注模式已关闭");
    }
}

// 再次推理
void detection::on_inferAgain_clicked()
{
    if (autoMode_)
    {
        emit statusMessageUpdate("重新进行推理");
        leftPartInstance->displayImage(leftPartInstance->getCurrentImagePath());
    }
    else
    {
        emit statusMessageUpdate("自动标注模式未开启");
    }
}

// 置信度
void detection::on_confidenceEdit_textChanged(const QString &text)
{
    bool ok;
    double confidence = text.toDouble(&ok);

    if (ok && confidence >= 0 && confidence <= 1)
    {
        confidence_threshold_ = confidence;
        emit statusMessageUpdate("置信度阈值已设置为: " + text);
    }
    else
    {
        emit statusMessageUpdate("输入无效, 请输入0-1之间的数值");
        if (!text.isEmpty())
        {
            ui->confidenceEdit->blockSignals(true);
            ui->confidenceEdit->setText(QString::number(confidence_threshold_));
            ui->confidenceEdit->blockSignals(false);
        }
    }
}

// nms
void detection::on_nmsEdit_textChanged(const QString &text)
{
    bool ok;
    double nms = text.toDouble(&ok);

    if (ok && nms >= 0 && nms <= 1)
    {
        nms_threshold_ = nms;
        emit statusMessageUpdate("nms阈值已设置为: " + text);
    }
    else
    {
        emit statusMessageUpdate("输入无效, 请输入0-1之间的数值");
        if (!text.isEmpty())
        {
            ui->nmsEdit->blockSignals(true);
            ui->nmsEdit->setText(QString::number(nms_threshold_));
            ui->nmsEdit->blockSignals(false);
        }
    }
}

// 模型选择
void detection::on_modelSelection_currentTextChanged(const QString &text)
{
    modelSelection_ = text.toStdString();
    if (modelSelection_ == "yolov8")
    {
        leftPartInstance->autoModeInstance->setModelPath(v8_model_path_);
    }
    else if (modelSelection_ == "yolov12")
    {
        leftPartInstance->autoModeInstance->setModelPath(v12_model_path_);
    }
}

// 保存颜色
void detection::on_colorSave_toggled(bool checked)
{
    colorSave_ = checked;

    auto buttons = colorSelection->buttons();
    for (auto button : buttons)
    {
        if (colorSave_)
        {
            emit statusMessageUpdate("颜色保存模式已启用");
            button->setCheckable(true);
            for (auto &label : detectionLabels_)
            {
                if (label.is_selected)
                {
                    if (label.color == "red")
                    {
                        ui->red->setChecked(true);
                    }
                    else if (label.color == "blue")
                    {
                        ui->blue->setChecked(true);
                    }
                }
            }
        }
        else
        {
            emit statusMessageUpdate("颜色保存模式已禁用");
            button->setChecked(false);
            button->setCheckable(false);
        }
    }

    updateLabelList();
    leftPartInstance->imageLabel->drawLabels();
}

// pointsMode
void detection::on_poseMode_toggled(bool checked)
{
    is_poseMode_ = checked;

    if (is_poseMode_)
    {
        emit statusMessageUpdate("四点标注模式已启用");

        for (auto &label : detectionLabels_)
        {
            if (!label.armor_points.empty())
            {
                label.is_pose = true;
            }
        }
    }
    else
    {
        emit statusMessageUpdate("四点标注模式已禁用");

        for (auto &label : detectionLabels_)
        {
            label.is_pose = false;
        }
    }

    updateLabelList();
    leftPartInstance->imageLabel->drawLabels();
}

// 颜色选择
void detection::onColorSelected(QAbstractButton *button)
{
    if (!colorSave_)
    {
        currentColor = "";
        return;
    }

    currentColor = button->text().toStdString();

    for (auto &label : detectionLabels_)
    {
        if (label.is_selected)
        {
            label.color = currentColor;
            leftPartInstance->imageLabel->drawLabels();
            updateLabelList();
            break;
        }
    }

    setFocus();
}

// 更新标签条目
void detection::updateLabelList()
{
    ui->labelList->clear();

    for (auto &label : detectionLabels_)
    {
        if (label.confidence < confidence_threshold_)
        {
            label.confidence = 0;
        }

        QString text;
        if (label.is_pose)
        {
            text = QString("[center: (%1, %2), lb: (%3, %4), lt: (%5, %6), rt: (%7, %8), rb: (%9, %10), conf: %11, color: %12")
                             .arg(label.center.x)               .arg(label.center.y)
                             .arg(label.armor_points[0].x)      .arg(label.armor_points[0].y)
                             .arg(label.armor_points[1].x)      .arg(label.armor_points[1].y)
                             .arg(label.armor_points[2].x)      .arg(label.armor_points[2].y)
                             .arg(label.armor_points[3].x)      .arg(label.armor_points[3].y)
                             .arg(QString::number(label.confidence, 'f', 2))
                             .arg(QString::fromStdString(label.color));
        }
        else
        {
            text = QString("[lx: %1, ly: %2, width: %3, height: %4, conf: %5, color: %6")
                             .arg(label.rect.x)
                             .arg(label.rect.y)
                             .arg(label.rect.width)
                             .arg(label.rect.height)
                             .arg(QString::number(label.confidence, 'f', 2))
                             .arg(QString::fromStdString(label.color));
        }

        if (!colorSave_)
        {
            text += "(已禁用)]";
        }
        else
        {
            text += "]";
        }

        QListWidgetItem *item = new QListWidgetItem(text);
        ui->labelList->addItem(item);

        if (label.is_selected)
        {
            item->setSelected(true);
        }
    }
}

// 标签点击
void detection::on_labelList_itemClicked(QListWidgetItem *item)
{
    int index = ui->labelList->row(item);

    if (index < 0 || index >= detectionLabels_.size() || currentLabelClickedId == index)
    {
        return;
    }

    for (int i = 0; i < ui->labelList->count(); i++)
    {
        ui->labelList->item(i)->setSelected(i == index);
    }

    for (auto &label : detectionLabels_)
    {
        label.is_selected = false;
    }

    detectionLabels_[index].is_selected = true;
    currentLabelClickedId = index;
    leftPartInstance->imageLabel->drawLabels();
    onDetectionLabelSelected(detectionLabels_[index]);

    updateLabelList();
}

// 选择标签--更新颜色选择和列表
void detection::onDetectionLabelSelected(detectionLabel &label)
{
    updateLabelList();
    setFocus();

    if (colorSave_)
    {
        QString color = QString::fromStdString(label.color);

        for (auto button : colorSelection->buttons())
        {
            if (button->text() == color)
            {
                button->setChecked(true);
                currentColor = label.color;
                break;
            }
        }
    }

    if (labelSave_)
    {
        ui->labelSelection->setCurrentText(QString::fromStdString(label.name));
        displayPreview(label.warp);
    }
}

// 创建标签
void detection::on_createLabel_clicked()
{
    if (!leftPartInstance || !leftPartInstance->imageLabel)
    {
        emit statusMessageUpdate("错误: 未初始化图像标签");
        return;
    }

    detectionLabel label;
    detectionLabel &tmpLabel = leftPartInstance->imageLabel->tmpLabel;

    if (tmpLabel.rect.empty())
    {
        emit statusMessageUpdate("保存错误: 未创建标签");
        return;
    }

    if (colorSave_ && currentColor.empty())
    {
        emit statusMessageUpdate("保存错误: 未选择颜色");
        return;
    }

    // 清除选中标签
    for (auto &label : detectionLabels_)
    {
        label.is_selected = false;
    }

    // 记录保存部分
    label.color = currentColor;
    label.rect = tmpLabel.rect;
    label.is_selected = true;
    labelSelected = true;
    label.is_saved = false;
    label.is_pose = false;

    cv::Mat originalImg = leftPartInstance->imageLabel->getCurrentImage();
    if (labelSave_ && is_warp_)
    {
        label.warp = originalImg(label.rect);
        label.name = labelId;
    }
    if (labelSave_ && is_binary_)
    {
        cv::Mat binary;
        cvtColor(label.warp, binary, cv::COLOR_BGR2GRAY);
        threshold(binary, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        label.warp = binary;
    }
    displayPreview(label.warp);

    int cx = label.rect.x + label.rect.width / 2.0;
    int cy = label.rect.y + label.rect.height / 2.0;
    label.center = cv::Point(cx, cy);

    detectionLabels_.push_back(label);
    tmpLabel = detectionLabel();

    leftPartInstance->imageLabel->drawLabels();
    updateLabelList();

    emit statusMessageUpdate("标签创建成功");
}

void detection::saveDetectionLabels()
{
    if (savePath_.isEmpty())
    {
        emit statusMessageUpdate("未设置保存路径");
        return;
    }

    if (!leftPartInstance || !leftPartInstance->imageLabel)
    {
        emit statusMessageUpdate("错误: 未初始化图像标签");
        return;
    }

    cv::Mat originalImg = leftPartInstance->imageLabel->originalImg;
    if (originalImg.empty())
    {
        emit statusMessageUpdate("错误: 图像为空");
        return;
    }

    QString currentImagePath = leftPartInstance->getCurrentImagePath();
    QFileInfo fileInfo(currentImagePath);
    QString originalFileName = fileInfo.baseName();

    // 创建标签目录
    QString labelsPath = savePath_ + "/labels";
    QDir labelDir(labelsPath);
    if (!labelDir.exists())
    {
        if (!labelDir.mkpath("."))
        {
            emit statusMessageUpdate("无法创建标签目录: " + labelsPath);
            return;
        }
    }

    // 创建图像目录
    QString imagesPath = savePath_ + "/images";
    QDir imageDir(imagesPath);
    if (labelSave_ && !imageDir.exists())
    {
        if (!imageDir.mkpath("."))
        {
            emit statusMessageUpdate("无法创建图像目录: " + imagesPath);
            return;
        }
    }

    // 打开标签文件
    QString saveFileName = labelsPath + "/" + originalFileName + ".txt";
    QFile file(saveFileName);
    // 尝试以只读和文本模式打开文件（不存在则创建，存在则清空内容）
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        emit statusMessageUpdate("无法打开文件: " + saveFileName);
        return;
    }

    QTextStream out(&file);
    int imgWidth = originalImg.cols;
    int imgHeight = originalImg.rows;
    int savedCount = 0;

    for (auto &label : detectionLabels_)
    {
        if (!label.is_saved)
        {
            // 保存标签文件
            int classId = 0;
            if (colorSave_)
            {
                if (label.color == "red")
                    classId = 0;
                else if (label.color == "blue")
                    classId = 1;
            }

            if (!label.is_pose)
            {
                // 计算归一化坐标
                double cx = label.center.x / imgWidth;
                double cy = label.center.y / imgHeight;
                double width = static_cast<double>(label.rect.width) / imgWidth;
                double height = static_cast<double>(label.rect.height) / imgHeight;

                // 写入文件
                out << classId << " " << cx << " " << cy << " " << width << " " << height << "\n";
            }
            else
            {
                // 计算包含所有关键点的最小矩形
                float min_x = FLT_MAX, max_x = FLT_MIN;
                float min_y = FLT_MAX, max_y = FLT_MIN;

                for (const auto& point : label.armor_points)
                {
                    if (point.x < min_x)
                        min_x = point.x;
                    if (point.x > max_x)
                        max_x = point.x;
                    if (point.y < min_y)
                        min_y = point.y;
                    if (point.y > max_y)
                        max_y = point.y;
                }

                double cx = (min_x + max_x) / 2.0 / imgWidth;
                double cy = (min_y + max_y) / 2.0 / imgHeight;
                double width = (max_x - min_x) / imgWidth;
                double height = (max_y - min_y) / imgHeight;

                // 写入文件
                out << classId << " " << cx << " " << cy << " " << width << " " << height;
                for (const auto& point : label.armor_points)
                {
                    double kpt_x = point.x / imgWidth;
                    double kpt_y = point.y / imgHeight;
                    // 可见性标志： 0 关键点不可见， 1 关键点被遮挡但位置可推断， 2 关键点可见且标注
                    out << " " << kpt_x << " " << kpt_y << " " << 2;
                }
                out << "\n";
            }

            // 保存图像文件
            if (labelSave_ && !label.warp.empty())
            {
                // 创建对应类的目录
                QString classDirPath = imagesPath + "/" + QString::fromStdString(label.name);
                QDir classDir(classDirPath);
                if (!classDir.exists())
                {
                    if (!classDir.mkpath("."))
                    {
                        emit statusMessageUpdate("无法创建图像目录: " + classDirPath);
                        return;
                    }
                }

                QString imageFileName = classDirPath + "/" + originalFileName + "_" + QString::number(savedCount) + ".jpg";
                QString fullImagePath = imageDir.filePath(imageFileName);
                cv::Mat img = label.warp;

                if (is_autoCut_ && width != 0 && height != 0)
                {
                    double scale = std::min(static_cast<double>(width) / img.cols, static_cast<double>(height) / img.rows);
                    int new_w = static_cast<int>(img.cols * scale);
                    int new_h = static_cast<int>(img.rows * scale);
                    cv::resize(img, img, cv::Size(new_w, new_h));

                    int padW = width - new_w;
                    int padH = height - new_h;

                    int left = padW / 2;
                    int top = padH / 2;
                    int right = padW - left;
                    int bottom = padH - top;

                    copyMakeBorder(img, img, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(125, 125, 125));
                }

                if (label.warp.channels() == 3)
                {
                    cvtColor(img, img, cv::COLOR_BGR2GRAY);
                }

                imwrite(fullImagePath.toStdString(), img);
            }
            else
            {
                emit statusMessageUpdate("图像保存失败");
            }

            label.is_saved = true;
            is_images_processed[currentImagePath] = true;
            savedCount++;
        }
    }

    file.close();

    if (savedCount > 0)
    {
        is_images_processed[currentImagePath] = true;
        emit statusMessageUpdate("成功保存 " + QString::number(savedCount) + " 个标签到 " + saveFileName);
    }
    else
    {
        emit statusMessageUpdate("未保存任何标签");
    }
}

void detection::on_labelSave_toggled(bool checked)
{
    labelSave_ = checked;

    if (labelSave_)
    {
        emit statusMessageUpdate("标签保存模式已启用");
        ui->binary->setEnabled(true);
        ui->warp->setEnabled(true);
        ui->widthEdit->setEnabled(true);
        ui->heightEdit->setEnabled(true);
        ui->saveWH->setEnabled(true);
        ui->labelSelection->setEnabled(true);

        if (!detectionLabels_.empty())
        {
            for (auto label : detectionLabels_)
            {
                if (label.is_selected)
                {
                    ui->labelSelection->setCurrentText(QString::fromStdString(label.name));
                    displayPreview(label.warp);
                    break;
                }
            }
        }
    }
    else
    {
        emit statusMessageUpdate("标签保存模式已禁用");
        ui->binary->setCheckable(false);
        ui->warp->setCheckable(false);
        ui->widthEdit->setEnabled(false);
        ui->heightEdit->setEnabled(false);
        ui->saveWH->setEnabled(false);
        ui->binary->setEnabled(false);
        ui->warp->setEnabled(false);
        ui->labelSelection->setEnabled(false);

        is_warp_ = false;
        is_binary_ = false;

        width = 0;
        height = 0;
    }
}

void detection::on_warp_toggled(bool checked)
{
    is_warp_ = checked;

    if (is_warp_)
    {
        if (!detectionLabels_.empty())
        {
            cv::Mat img = leftPartInstance->imageLabel->getCurrentImage();
            for (auto &label : detectionLabels_)
            {
                if (!label.armor_points.empty())
                {
                    label.warp = leftPartInstance->autoModeInstance->warp(img, label.armor_points);
                    if (label.is_selected)
                    {
                        displayPreview(label.warp);
                    }
                }
            }
        }
        emit statusMessageUpdate("保存warp后图片");
    }
    else
    {
        emit statusMessageUpdate("关闭warp模式");
    }
}

void detection::on_binary_toggled(bool checked)
{
    is_binary_ = checked;

    if (is_binary_)
    {
        if (!detectionLabels_.empty())
        {
            cv::Mat img = leftPartInstance->imageLabel->getCurrentImage();
            for (auto &label : detectionLabels_)
            {
                if (label.warp.channels() != 1)
                {
                    cvtColor(label.warp, label.warp, cv::COLOR_RGB2GRAY);
                }
                threshold(label.warp, label.warp, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
                cvtColor(label.warp, label.warp, cv::COLOR_GRAY2RGB);

                if (label.is_selected)
                {
                    displayPreview(label.warp);
                }
            }
        }
        emit statusMessageUpdate("保存binary后图片");
    }
    else
    {
        emit statusMessageUpdate("关闭binary模式");
    }
}

void detection::on_labelSelection_currentTextChanged(const QString &text)
{
    labelId = text.toStdString();
    emit statusMessageUpdate("当前选择的标签: " + text);
}

void detection::on_saveWH_clicked()
{
    is_autoCut_ = true;

    QString widthString = ui->widthEdit->text();
    QString heightString = ui->heightEdit->text();

    if (widthString.toInt() > 0 && heightString.toInt() > 0)
    {
        width = widthString.toInt();
        height = heightString.toInt();
        emit statusMessageUpdate("自动裁剪模式：裁剪大小为 " + widthString + "x" + heightString);
    }
    else
    {
        ui->widthEdit->setText("");
        ui->heightEdit->setText("");
        emit statusMessageUpdate("自动裁剪模式: 裁剪大小无效");
    }
}

void detection::displayPreview(cv::Mat img)
{
    if (img.empty())
    {
        ui->previewLabel->clear();
        emit statusMessageUpdate("预览图像为空");
        return;
    }

    if (img.channels() == 1)
    {
        cvtColor(img, img, cv::COLOR_GRAY2BGR);
    }

    QImage qImg(img.data, img.cols, img.rows, img.step, QImage::Format_RGB888);

    QPixmap scaled_pixmap = QPixmap::fromImage(qImg).scaled(
        ui->previewLabel->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);

    ui->previewLabel->setPixmap(scaled_pixmap);
}

void detection::makePoseLabel(std::vector<cv::Point> &posePoints)
{
    if (posePoints.size() != 4)
    {
        emit statusMessageUpdate("错误: 未检测到正确数量的关键点");
        return;
    }

    // 求伪中心点
    cv::Point2f tmpCenter;
    for (auto &point : posePoints)
    {
        tmpCenter.x += point.x;
        tmpCenter.y += point.y;
    }
    tmpCenter.x /= 4;

    std::vector<cv::Point2f> left;
    std::vector<cv::Point2f> right;
    cv::Point2f lb, lt, rt, rb;

    for (auto &point : posePoints)
    {
        if (point.x < tmpCenter.x)
        {
            left.push_back(point);
        }
        else
        {
            right.push_back(point);
        }
    }

    // 左上角是原点
    left[0].y > left[1].y ? (lb = left[0], lt = left[1]) : (lb = left[1], lt = left[0]);
    right[0].y > right[1].y ? (rb = right[0], rt = right[1]) : (rb = right[1], rt = right[0]);

    std::vector<cv::Point2f> armor_points = {lb, lt, rt, rb};

    // 几何中心点
    cv::Point2f center = getCenterFromPose(armor_points);

    // 计算边界框
    float xmin = FLT_MAX, xmax = FLT_MIN;
    float ymin = FLT_MAX, ymax = FLT_MIN;
    for (auto &point : armor_points)
    {
        xmin = std::min(xmin, point.x);
        xmax = std::max(xmax, point.x);
        ymin = std::min(ymin, point.y);
        ymax = std::max(ymax, point.y);
    }
    cv::Rect rect = cv::Rect(xmin, ymin, xmax - xmin, ymax - ymin);

    detectionLabel poseLabel;
    poseLabel.armor_points = armor_points;
    poseLabel.center = center;
    poseLabel.color = currentColor;
    poseLabel.confidence = 1.0;
    poseLabel.is_saved = false;
    poseLabel.is_selected = true;
    poseLabel.is_pose = true;
    labelSelected = true;
    cv::Rect roi = rect;
    poseLabel.rect = roi;
    if (labelSave_)
    {
        if (is_warp_)
        {
            cv::Mat originalImg = leftPartInstance->imageLabel->getCurrentImage();
            cv::Mat num_img = leftPartInstance->autoModeInstance->warp(originalImg, armor_points);
            if (num_img.empty())
            {
                num_img = originalImg(poseLabel.rect);
            }
            poseLabel.warp = num_img;
        }
        if (is_binary_)
        {
            cv::Mat binary;
            cvtColor(poseLabel.warp, binary, cv::COLOR_BGR2GRAY);
            threshold(binary, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
            cvtColor(binary, poseLabel.warp, cv::COLOR_GRAY2BGR);
        }
        if (!poseLabel.warp.empty())
        {
            displayPreview(poseLabel.warp);
        }
        poseLabel.name = labelId;
    }

    detectionLabels_.push_back(poseLabel);

    leftPartInstance->imageLabel->drawLabels();
    updateLabelList();
    setFocus();
}

void detection::keyPressEvent(QKeyEvent *event)
{
    if (labelSave_ && event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9)
    {
        int index = event->key() - Qt::Key_1;
        if (index >= 0 && index < ui->labelSelection->count())
        {
            ui->labelSelection->setCurrentIndex(index);
            updateLabelList();
            leftPartInstance->imageLabel->drawLabels();
        }
    }
    else if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        leftPartInstance->imageLabel->setFocus();
    }
    else if (colorSave_)
    {
        if (event->key() == Qt::Key_Q)
        {
            ui->red->setChecked(true);
            currentColor = "red";
            onColorSelected(ui->red);
        }
        else if (event->key() == Qt::Key_E)
        {
            ui->blue->setChecked(true);
            currentColor = "blue";
            onColorSelected(ui->blue);
        }
    }
    else
    {
        QWidget::keyPressEvent(event);
    }
}
