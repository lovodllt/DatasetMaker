#include "cls.h"
#include "ui_cls.h"

cls::cls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::cls),
    leftPartInstance(nullptr)
{
    ui->setupUi(this);

    previewLabel = ui->previewLabel;
    QButtonGroup *classButtonGroup = ui->classButtonGroup;

    ui->widthEdit->setEnabled(false);
    ui->heightEdit->setEnabled(false);

    connect(classButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &cls::onClassSelected);
    connect(ui->autoCut, &QRadioButton::toggled, [this](bool checked) {
        ui->widthEdit->setEnabled(checked);
        ui->heightEdit->setEnabled(checked);
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
}

// 类别选择
void cls::onClassSelected(QAbstractButton *button)
{
    QString classId = button->text();
    clsname = classId.toStdString();
}

// 宽高
void cls::on_sure_clicked()
{
    is_autoCut_ = ui->autoCut->isChecked();

    QString widthString = ui->widthEdit->text();
    QString heightString = ui->heightEdit->text();

    if (is_autoCut_)
    {
        width = widthString.toInt();
        height = heightString.toInt();
        emit statusMessageUpdate("自动裁剪模式: 裁剪大小为 " + widthString + "x" + heightString);
    }
    else
    {
        emit statusMessageUpdate("自动裁剪未启用");
    }

}

// 保存标签
void cls::on_saveLabel_clicked()
{
    if (!leftPartInstance || !leftPartInstance->imageLabel)
    {
        emit statusMessageUpdate("错误: 未初始化图像标签");
        return;
    }

    detectionLabel label;
    detectionLabel &tmpLabel = leftPartInstance->imageLabel->tmpLabel;

    if (!tmpLabel.rect.empty())
    {
        // 记录保存部分
        label.name = clsname;
        label.rect = tmpLabel.rect;
        label.is_selected = true;

        detectionLabels_.push_back(label);
        tmpLabel = detectionLabel();

        leftPartInstance->imageLabel->drawLabels();

        QString message = tr("标签模式: 标签创建成功");
        emit statusMessageUpdate("标签创建成功");
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
    auto showPreview = [this, &originalImg](const cv::Rect &rect)
    {
        cv::Mat img = originalImg(rect);
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
        showPreview(leftPartInstance->imageLabel->tmpLabel.rect);
    }
    else if (!detectionLabels_.empty())
    {
        for (const auto &label : detectionLabels_)
        {
            if (label.is_selected)
            {
                showPreview(label.rect);
            }
        }
    }
    else
    {
        previewLabel->clear();
    }
}



