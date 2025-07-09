#include "detection.h"
#include "ui_detection.h"

detection::detection(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::detection),
    leftPartInstance(nullptr)
{
    ui->setupUi(this);
    is_warp_ = false;

    colorSelection = ui->colorSelection;

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

        QString text = QString("[lx: %1, ly: %2, width: %3, height: %4, conf: %5, color: %6")
                              .arg(label.rect.x)
                              .arg(label.rect.y)
                              .arg(label.rect.width)
                              .arg(label.rect.height)
                              .arg(label.confidence)
                              .arg(QString::fromStdString(label.color));

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

    if (index < 0 || index >= detectionLabels_.size())
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
    leftPartInstance->imageLabel->drawLabels();
    onDetectionLabelSelected(detectionLabels_[index]);

    updateLabelList();
}

// 选择标签--更新颜色选择和列表
void detection::onDetectionLabelSelected(detectionLabel &label)
{
    updateLabelList();

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

    if (is_binary_)
    {
        cv::Mat binary;
        cvtColor(label.warp, binary, cv::COLOR_BGR2GRAY);
        threshold(binary, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        label.warp = binary;
    }

    detectionLabels_.push_back(label);
    tmpLabel = detectionLabel();

    leftPartInstance->imageLabel->drawLabels();

    emit statusMessageUpdate("标签创建成功");

    updateLabelList();
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

    QString currentImagePath = leftPartInstance->getCurrentImagePath();
    QFileInfo fileInfo(currentImagePath);
    QString originalFileName = fileInfo.baseName();
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
            // 计算归一化坐标
            double cx = (label.rect.x + label.rect.width / 2.0) / imgWidth;
            double cy = (label.rect.y + label.rect.height / 2.0) / imgHeight;
            double width = static_cast<double>(label.rect.width) / imgWidth;
            double height = static_cast<double>(label.rect.height) / imgHeight;
            std::cout<<"width-first"<<label.rect.width<<"   "<<"height-first"<<label.rect.height<<std::endl;
            std::cout<<"width-last"<<width<<"   "<<"height-last"<<height<<std::endl;

            int classId = 0;
            if (colorSave_)
            {
                if (label.color == "red")
                    classId = 0;
                else if (label.color == "blue")
                    classId = 1;
            }
            else
            {
                classId = 0;
            }

            // 写入文件
            out << classId << " " << cx << " " << cy << " " << width << " " << height << "\n";

            savedCount++;
        }
    }

    file.close();

    if (savedCount > 0)
    {
        emit statusMessageUpdate("成功保存 " + QString::number(savedCount) + " 个标签到 " + saveFileName);
    }
    else
    {
        emit statusMessageUpdate("未保存任何标签");
    }
}
