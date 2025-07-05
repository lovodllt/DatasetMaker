#include "leftPart.h"
#include "cls.h"
#include "ui_leftPart.h"

leftPart::leftPart(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::leftPart)
{
    ui->setupUi(this);

    imageArea = ui->imageArea;

    // 创建图片展示
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
        processDirectory(dirPath);is_labeling_ = ! is_labeling_;
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
        markProcessedImages(savePath_);

        QString message = tr("保存路径: %1 | 格式: %2").arg(savePath_).arg(saveFormat_);
        emit statusMessageUpdate(message);
    }
}

// 标记已处理的图像
void leftPart::markProcessedImages(const QString &savePath)
{
    QDir saveDir(savePath);
    if (!saveDir.exists()) return;

    // 获取保存目录下的所有文件名
    QStringList savedFiles;
    QDirIterator it(savePath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        savedFiles << QFileInfo(it.next()).fileName();
    }

    // 遍历所有图像文件
    for (int i = 0; i < imageFiles.size(); i++)
    {
        QString originalFileName = imageFiles[i].baseName();
        bool is_processed = false;

        for (const auto &savedFile : savedFiles)
        {
            if (savedFile.startsWith(originalFileName))
            {
                is_processed = true;
                break;
            }
        }

        if (is_processed)
        {
            is_images_processed[imageFiles[i].absoluteFilePath()] = true;
            ui->imageList->item(i)->setCheckState(Qt::Checked);
        }
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

    saveCurrentLabels();

    saveId_ = 0;
    int newIndex = (currentIndex + 1) % imageFiles.size();
    setCurrentImage(newIndex);
}

void leftPart::on_prevImage_clicked()
{
    if (imageFiles.isEmpty()) return;

    saveCurrentLabels();

    saveId_ = 0;
    int newIndex = (currentIndex - 1) % imageFiles.size();
    setCurrentImage(newIndex);
}

void leftPart::on_createLabel_clicked()
{
    if (originalPixmap.isNull())
    {
        emit statusMessageUpdate("进入标签模式失败，请先选择图片");
        return;
    }

    if (!is_labeling_)
    {
        is_labeling_ = true;
        emit statusMessageUpdate("进入标签模式");

        ui->createLabel->setStyleSheet(
            "QPushButton {"
            "    border: 1px solid #c0c0c0;"
            "    border-radius: 5px;"
            "    background: #d0d0d0;"
            "    padding: 5px;"
            "}"
            "QPushButton:hover {"
            "    background: #e0e0e0;"
            "}"
            "QPushButton:pressed {"
            "    background: #d0d0d0;"
            "}"
            );
    }
    else
    {
        if (!imageLabel->tmpLabel.rect.empty())
        {
            statusMessageUpdate("退出标签模式失败：请先清除临时标签");
            return;
        }

        if (!detectionLabels_.empty())
        {
            if (savePath_.isEmpty())
            {
                saveFilePath();
            }

            bool needSave = false;
            for (auto &label : detectionLabels_)
            {
                if (!label.is_saved)
                {
                    needSave = true;
                    break;
                }
            }

            if (needSave)
            {
                saveCurrentLabels();
            }
        }

        imageLabel->clearLabels();

        is_labeling_ = false;
        emit statusMessageUpdate("退出标签模式");

        ui->createLabel->setStyleSheet(
            "QPushButton {"
            "    border: 1px solid #c0c0c0;"
            "    border-radius: 5px;"
            "    background: #f0f0f0;"
            "    padding: 5px;"
            "}"
            "QPushButton:hover {"
            "    background: #e0e0e0;"
            "}"
            "QPushButton:pressed {"
            "    background: #d0d0d0;"
            "}"
        );
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
    if (currentIndex < 0 || currentIndex >= imageFiles.size())
    {
        emit statusMessageUpdate("没有可删除文件");
        return;
    }

    QFileInfo currentFile = imageFiles[currentIndex];
    QString currentFileName = currentFile.baseName();

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除",
                           "确认要删除此图片的标签文件吗？",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
    {
        return;
    }

    if (!savePath_.isEmpty())
    {
        QDir saveDir(savePath_);
        if (saveDir.exists())
        {
            QDirIterator it(savePath_, QStringList() << currentFileName + "_*", QDir::Files, QDirIterator::Subdirectories);
            int deleteCount = 0;

            while (it.hasNext())
            {
                if (QFile::remove(it.next()))
                {
                    ui->imageList->item(currentIndex)->setCheckState(Qt::Unchecked);
                    deleteCount++;
                }
            }

            if (deleteCount > 0)
            {
                QString message = tr("已删除 %1 个标签文件").arg(deleteCount);
                emit statusMessageUpdate(message);
            }
        }
    }
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

// 获取当前图像路径
QString leftPart::getCurrentImagePath()
{
    if (currentIndex >= 0 && currentIndex < imageFiles.size())
    {
        return imageFiles[currentIndex].absoluteFilePath();
    }
    return QString();
}

// 保存标签文件
void leftPart::saveCurrentLabels()
{
    if (savePath_.isEmpty())
    {
        saveFilePath();
    }

    if (!imageLabel->tmpLabel.rect.empty())
    {
        statusMessageUpdate("请先清除临时标签");
        return;
    }

    if (is_labeling_ && !detectionLabels_.empty())
    {
        bool needSave = false;
        for (auto &label : detectionLabels_)
        {
            if (!label.is_saved)
            {
                needSave = true;
                break;
            }
        }

        if (needSave)
        {
            if (labelMode_ == "cls")
            {
                clsInstance->saveClsLabels();
            }

            ui->imageList->item(currentIndex)->setCheckState(Qt::Checked);
        }
    }
}
