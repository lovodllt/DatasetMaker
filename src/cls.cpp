#include "cls.h"
#include "ui_cls.h"

cls::cls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::cls),
    leftPartInstance(nullptr)
{
    ui->setupUi(this);

    previewLabel = ui->previewLabel;
    classButtonGroup = ui->classButtonGroup;

    is_autoCut_ = ui->autoCut->isChecked();
    ui->widthEdit->setEnabled(false);
    ui->heightEdit->setEnabled(false);

    connect(classButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &cls::onClassSelected);
    connect(ui->autoCut, &QRadioButton::toggled, [this](bool checked) {
        is_autoCut_ = checked;
        ui->widthEdit->setEnabled(checked);
        ui->heightEdit->setEnabled(checked);
        emit statusMessageUpdate(checked ? "自动裁剪模式已启用" : "自动裁剪模式已禁用");
    });
}

cls::~cls()
{
    delete ui;
}

// 获取左侧功能栏指针
void cls::setLeftPart(leftPart *leftPart)
{
    this->leftPartInstance = leftPart;
    leftPartInstance->clsInstance = this;
}

// 类别选择
void cls::onClassSelected(QAbstractButton *button)
{
    QString classId = button->text();
    clsname = classId.toStdString();

    for (auto &label : detectionLabels_)
    {
        if (label.is_selected)
        {
            label.name = clsname;
            leftPartInstance->imageLabel->drawLabels();
            break;
        }
    }
}

// 宽高
void cls::on_sure_clicked()
{
    is_autoCut_ = ui->autoCut->isChecked();

    QString widthString = ui->widthEdit->text();
    QString heightString = ui->heightEdit->text();

    if (is_autoCut_)
    {
        if (widthString.toInt() > 0 && heightString.toInt() > 0)
        {
            width = widthString.toInt();
            height = heightString.toInt();
            emit statusMessageUpdate("自动裁剪模式: 裁剪大小为 " + widthString + "x" + heightString);
        }
        else
        {
            ui->widthEdit->setText("0");
            ui->heightEdit->setText("0");
            emit statusMessageUpdate("自动裁剪模式: 裁剪大小无效");
        }
    }
    else
    {
        emit statusMessageUpdate("自动裁剪未启用");
    }
}

// 保存标签
void cls::on_createLabel_clicked()
{
    if (!leftPartInstance || !leftPartInstance->imageLabel)
    {
        emit statusMessageUpdate("错误: 未初始化图像标签");
        return;
    }

    detectionLabel label;
    detectionLabel &tmpLabel = leftPartInstance->imageLabel->tmpLabel;

    if (is_autoCut_ && (width == 0 || height == 0))
    {
        emit statusMessageUpdate("保存错误: 未设置裁剪大小");
        return;
    }

    if (clsname.empty())
    {
        emit statusMessageUpdate("保存错误: 未选择类别");
        return;
    }

    if (tmpLabel.rect.empty())
    {
        emit statusMessageUpdate("保存错误: 未创建标签");
        return;
    }

    // 清除选中标签
    for (auto &label : detectionLabels_)
    {
        label.is_selected = false;
    }

    // 记录保存部分
    label.name = clsname;
    label.rect = tmpLabel.rect;
    label.is_selected = true;

    detectionLabels_.push_back(label);
    tmpLabel = detectionLabel();

    leftPartInstance->imageLabel->drawLabels();

    emit statusMessageUpdate("标签创建成功");
}

// 创建保存文件名
QString cls::createFileName(const QString &originalFileName)
{
    // 生成文件名: 原始文件名_索引.jpg
    saveId_++;
    return QString("%1_%2.jpg").arg(originalFileName).arg(saveId_);
}

// 创建文件夹并保存裁剪后的图像
void cls::saveCroppedImage(const cv::Mat &crop, const QString &originalFileName, const QString &className)
{
    // 创建文件夹
    QString classDirPath = savePath_ + "/" + className;
    QDir classDir(classDirPath);
    if (!classDir.exists())
    {
        if (!classDir.mkpath("."))
        {
            emit statusMessageUpdate("创建文件夹失败: " + classDirPath);
            return;
        }
    }

    QString fileName = createFileName(originalFileName);
    QString fullPath = classDir.filePath(fileName);

    // 调整图像大小
    cv::Mat resizedcrop;
    if (is_autoCut_ && width != 0 && height != 0)
    {
        double scale = std::min(static_cast<double>(width) / crop.cols, static_cast<double>(height) / crop.rows);
        int new_w = static_cast<int>(crop.cols * scale);
        int new_h = static_cast<int>(crop.rows * scale);
        cv::resize(crop, resizedcrop, cv::Size(new_w, new_h));

        int padW = width - new_w;
        int padH = height - new_h;

        int left = padW / 2;
        int top = padH / 2;
        int right = padW - left;
        int bottom = padH - top;

        copyMakeBorder(resizedcrop, resizedcrop, top, bottom, left, right, cv::BORDER_CONSTANT, cv::Scalar(125, 125, 125));
    }
    else
    {
        resizedcrop = crop;
    }

    cv::cvtColor(resizedcrop, resizedcrop, cv::COLOR_BGR2RGB);
    if (!cv::imwrite(fullPath.toStdString(), resizedcrop))
    {
        emit statusMessageUpdate("保存图像失败: " + fullPath);
        return;
    }
}

// 保存标签文件
void cls::saveClsLabels()
{
    if (savePath_.isEmpty())
    {
        emit statusMessageUpdate("保存路径未设置");
        return;
    }

    if (!leftPartInstance || !leftPartInstance->imageLabel)
    {
        emit statusMessageUpdate("错误: 未初始化图像标签");
        return;
    }

    cv::Mat originalImg = leftPartInstance->imageLabel->originalImg.clone();
    if (originalImg.empty())
    {
        emit statusMessageUpdate("错误: 图像为空");
        return;
    }

    QString currentImagePath = leftPartInstance->getCurrentImagePath();
    QFileInfo fileInfo(currentImagePath);
    QString originalFileName = fileInfo.baseName();

    int savedCount = 0;

    for (auto &label : detectionLabels_)
    {
        if (!label.is_saved)
        {
            cv::Mat crop;
            if (is_warp_)
            {
                crop = label.warp;
                if (crop.empty())
                {
                    emit statusMessageUpdate("错误: warp图像为空");
                    crop = originalImg(label.rect);
                }
            }
            else
            {
                crop = originalImg(label.rect);
            }
            saveCroppedImage(crop, originalFileName, QString::fromStdString(label.name));
            label.is_saved = true;
            savedCount++;
        }
    }

    if (savedCount > 0)
    {
        emit statusMessageUpdate("已保存 " + QString::number(savedCount) + " 张图像");
    }
    else
    {
        emit statusMessageUpdate("没有需要保存的标签");
    }
}

// 展示预览图
void cls::displayPreview()
{
    // 检查leftPartInstance和imageLabel是否有效
    if (!leftPartInstance || !leftPartInstance->imageLabel)
    {
        previewLabel->clear();
        return;
    }

    cv::Mat originalImg = leftPartInstance->imageLabel->originalImg.clone();

    // 预览标签的lamba函数
    auto showPreview = [this, &originalImg](const cv::Mat img)
    {
        QImage qImg(img.data, img.cols, img.rows, img.step, QImage::Format_RGB888);

        QPixmap scaledPixmap = QPixmap::fromImage(qImg).scaled(
            previewLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        previewLabel->setPixmap(scaledPixmap);
    };

    // 优先预览临时标签, 其次预览选中标签
    if (!leftPartInstance->imageLabel->tmpLabel.rect.empty())
    {
        cv::Mat img = originalImg(leftPartInstance->imageLabel->tmpLabel.rect);
        showPreview(img);
    }
    else if (!detectionLabels_.empty())
    {
        for (const auto &label : detectionLabels_)
        {
            if (label.is_selected)
            {
                if (is_warp_)
                {
                    showPreview(label.warp);
                }
                else
                {
                    cv::Mat img = originalImg(label.rect);
                    showPreview(img);
                }
            }
        }
    }
    else
    {
        previewLabel->clear();
    }
}

// 自动模式
void cls::on_autoMode_toggled(bool checked)
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

// 保存warp图片
void cls::on_warp_toggled(bool checked)
{
    if (checked)
    {
        is_warp_ = true;
        emit statusMessageUpdate("保存warp后图片");
    }
    else
    {
        is_warp_ = false;
        emit statusMessageUpdate("保存原始图片");
    }
}

// 置信度阈值更新
void cls::on_confidenceEdit_textChanged(const QString &text)
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

// nms阈值更新
void cls::on_nmsEdit_textChanged(const QString &text)
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
void cls::on_modelSelection_currentTextChanged(const QString &text)
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

// 标签类别展示和更新
void cls::onLabelSelected(detectionLabel &label)
{
    QString className = QString::fromStdString(label.name);

    for (auto button : classButtonGroup->buttons())
    {
        if (button->text() == className)
        {
            button->setChecked(true);
            clsname = label.name;
            break;
        }
    }
}
