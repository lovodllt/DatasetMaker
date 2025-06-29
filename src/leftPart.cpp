#include "leftPart.h"
#include "ui_leftPart.h"

leftPart::leftPart(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::leftPart)
{
    ui->setupUi(this);

    imageArea = ui->imageArea;
    imageArea->setWidgetResizable(false);
    imageArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    imageArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    imageArea->setAlignment(Qt::AlignCenter);

    // 创建labelPainter实例并设置为滚动区域的widget
    imageLabel = new ImageLabel();
    imageLabel->leftPartInstance = this;
    imageLabel->currentScale = 1.0;
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(false);
    imageLabel->setStyleSheet("QLabel { color: #808080; font-size: 18px; }");
    imageLabel->setText("请选择图片");

    imageArea->setWidget(imageLabel);

    // 设置分割器初始大小
    ui->outerSplitter->setSizes({200, 500});
    ui->leftSplitter->setSizes({150, 450});

    // 设置分割器比例
    ui->outerSplitter->setStretchFactor(0, 1); // 左侧按钮/列表区域
    ui->outerSplitter->setStretchFactor(1, 3); // 右侧图像区域

    ui->leftSplitter->setStretchFactor(0, 1);  // 上方按钮区域
    ui->leftSplitter->setStretchFactor(1, 2);  // 下方图像列表


    // 连接列表点击信号
    connect(ui->imageList, &QListWidget::itemClicked, this, &leftPart::on_imageListWidget_itemClicked);

    // 设置焦点
    setFocusPolicy(Qt::StrongFocus);
}

leftPart::~leftPart()
{
    delete ui;
}

// 打开文件
void leftPart::openFile()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        tr("选择文件夹"),
        "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dirPath.isEmpty())
    {
        QString message = tr("已选择文件夹: %1").arg(dirPath);
        emit statusMessageUpdate(message);
        processDirectory(dirPath);
    }
}

// 文件处理
void leftPart::processDirectory(const QString& dirPath)
{
    QDir dir(dirPath);
    QStringList filters{"*.png", "*.jpg", "*.jpeg"};   // 文件过滤器
    QFileInfoList imageFiles = dir.entryInfoList(filters, QDir::Files);

    QMap<QString, bool> processedMap;
    for (const auto &file : imageFiles)
    {
        processedMap[file.absoluteFilePath()] = false;
    }
    populateImageList(imageFiles, processedMap);
}

// 选择保存路径
void leftPart::saveFilePath()
{
    savePath_ = QFileDialog::getExistingDirectory(
        this,
        tr("选择保存路径"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!savePath_.isEmpty())
    {
        QString message = tr("保存路径: %1 | 格式: %2").arg(savePath_).arg(saveFormat_);
        emit statusMessageUpdate(message);
    }
}

// 按钮功能实现
void leftPart::on_openDir_clicked()
{
    openFile();
}

void leftPart::on_nextImage_clicked()
{
    if (imageFiles.isEmpty()) return;

    int newIndex = (currentIndex + 1) % imageFiles.size();
    setCurrentImage(newIndex);

    if (is_labeling_)
    {
        if (!imageLabel->tmpLabel.rect.empty())
        {
            // 保存标签
        }

        imageLabel->clearLabels();

        // 读取新的标签
    }
}

void leftPart::on_prevImage_clicked()
{
    if (imageFiles.isEmpty()) return;

    int newIndex = (currentIndex - 1) % imageFiles.size();
    setCurrentImage(newIndex);

    if (is_labeling_)
    {
        if (!imageLabel->tmpLabel.rect.empty())
        {
            // 保存标签
        }

        imageLabel->clearLabels();

        // 读取新的标签
    }
}

void leftPart::on_createLabel_clicked()
{
    if (originalPixmap.isNull())
    {
        QString message = tr("进入标签模式失败，请先选择图片");
        emit statusMessageUpdate(message);
        return;
    }
    else
    {
        is_labeling_ = ! is_labeling_;
    }

    if (is_labeling_)
    {
        QString message = tr("进入标签模式");
        emit statusMessageUpdate(message);
    }
    else
    {
        if (!imageLabel->tmpLabel.rect.empty())
        {
            // 保存标签
        }
        imageLabel->clearLabels();
        QString message = tr("退出标签模式");
        emit statusMessageUpdate(message);
    }
}

void leftPart::on_save_clicked()
{
    if (savePath_.isEmpty())
    {
        saveFilePath();
    }
}

void leftPart::on_deleteFile_clicked()
{

}

// 创建图像文件条目
void leftPart::populateImageList(const QFileInfoList &imageFiles, QMap<QString, bool> &is_images_processed)
{
    this->imageFiles = imageFiles;
    this->is_images_processed = is_images_processed;
    currentIndex = 0;

    ui->imageList->clear();

    for (const QFileInfo &fileInfo : imageFiles)
    {
        QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setCheckState(Qt::Unchecked);
        ui->imageList->addItem(item);
    }

    if (!imageFiles.isEmpty())
    {
        setCurrentImage(0);
    }
}

// 图像文件条目点击处理
void leftPart::on_imageListWidget_itemClicked(QListWidgetItem *item)
{
    int index = ui->imageList->row(item);
    if (index >= 0 && index < imageFiles.size())
    {
        setCurrentImage(index);
    }
}

// 图像索引更新并展示
void leftPart::setCurrentImage(int index)
{
    if (index < 0 || index >= imageFiles.size()) return;

    currentIndex = index;
    displayImage(imageFiles[index].absoluteFilePath());

    // 高亮当前选择
    for (int i = 0; i < ui->imageList->count(); i++)
    {
        ui->imageList->item(i)->setSelected(i == currentIndex);
    }

    // 更新图像条目位置
    ui->imageList->scrollToItem(ui->imageList->item(currentIndex));
}

// 图像展示
void leftPart::displayImage(const QString &imagePath)
{
    QImage image(imagePath);
    if (image.isNull()) return;

    // 清除之前的标签
    detectionLabels_.clear();

    originalPixmap = QPixmap::fromImage(image);
    originalImageSize = originalPixmap.size();

    // 计算缩放比例以适应窗口
    QSize viewportSize = imageArea->viewport()->size();
    double ratio = qMin(
        static_cast<double>(viewportSize.width()) / originalImageSize.width(),
        static_cast<double>(viewportSize.height()) / originalImageSize.height()
    );

    // 记录新的缩放比例
    imageLabel->currentScale = ratio;

    // 创建缩放后的图像
    QSize scaledSize = originalImageSize * ratio;
    QPixmap scaledPixmap = originalPixmap.scaled(
        scaledSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    // 设置图像到标签
    imageLabel->setPixmap(scaledPixmap);
    imageLabel->resize(scaledSize);
}

// 滚轮事件
void leftPart::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier && !originalPixmap.isNull())
    {
        double factor = event->angleDelta().y() > 0 ? zoomFactor : 1 / zoomFactor;

        // 获取当前缩放比例（基于原始图像尺寸）
        QSizeF originalSize = originalImageSize;  // 原始图像尺寸
        QSizeF currentSize = imageLabel->size();  // 视图尺寸
        currentScale = currentSize.width() / originalSize.width();

        // 计算新缩放比例并限制范围
        double newScale = currentScale * factor;
        newScale = qMax(0.1, qMin(newScale, 10.0));

        // 记录新的缩放比例
        imageLabel->currentScale = newScale;

        // 计算新图像尺寸
        QSize newsize = (originalSize * newScale).toSize();
        QPixmap newPixmap = originalPixmap.scaled(
            newsize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        // 更新图像显示
        imageLabel->setPixmap(newPixmap);
        imageLabel->resize(newsize);

        // 获取鼠标在视口中的位置
        QPointF viewportMousePos = imageArea->viewport()->mapFrom(this, event->pos());
        QPoint localMousePos = imageLabel->mapFromParent(imageArea->viewport()->mapToParent(viewportMousePos.toPoint()));

        // 计算鼠标在当前图像上的相对位置比例
        double xRatio = localMousePos.x() / currentSize.width();
        double yRatio = localMousePos.y() / currentSize.height();

        // 计算新图像上对应点的位置
        double newPointX = newsize.width() * xRatio;
        double newPointY = newsize.height() * yRatio;

        // 计算新滚动条位置
        int newHScroll = static_cast<int>(newPointX - viewportMousePos.x());
        int newVScroll = static_cast<int>(newPointY - viewportMousePos.y());

        // 设置滚动条位置
        imageArea->horizontalScrollBar()->setValue(newHScroll);
        imageArea->verticalScrollBar()->setValue(newVScroll);

        imageLabel->drawLabels();

        event->accept();
    }
    else
    {
        QWidget::wheelEvent(event);
    }
}