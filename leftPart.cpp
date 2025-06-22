#include "leftPart.h"
#include "ui_leftPart.h"

QString savePath_;
QString saveFormat_ = "yolo";
QString labelMode_;

leftPart::leftPart(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::leftPart)
{
    ui->setupUi(this);

    imageArea = ui->imageArea;
    imageArea->setWidgetResizable(true);
    imageArea->setAlignment(Qt::AlignCenter);

    // 创建labelPainter实例并设置为滚动区域的widget
    imageLabel = new ImageLabel();
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(false);
    imageLabel->setStyleSheet("QLabel { color: #808080; font-size: 18px; }");
    imageLabel->setText("请选择图片");

    QLayout *layout = imageArea->widget()->layout();
    if (layout)
    {
        QLayoutItem *item = layout->takeAt(0);
        if (item) delete item->widget();
        delete item;
        layout->addWidget(imageLabel);
    }

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
}

void leftPart::on_prevImage_clicked()
{
    if (imageFiles.isEmpty()) return;

    int newIndex = (currentIndex - 1) % imageFiles.size();
    setCurrentImage(newIndex);
}

void leftPart::on_createLabel_clicked()
{
    is_labeling = !is_labeling;
    ui->createLabel->setChecked(is_labeling);

    if (is_labeling)
    {
        emit statusMessageUpdate("标签模式: 点击图像设置起点");
        imageLabel->clearFirstPoint();
        imageLabel->clearPreview();
        isDrawingRect = false;
    }
    else
    {
        emit statusMessageUpdate("标签模式已退出");
        firstPoint = QPoint();
        imageLabel->clearFirstPoint();
        imageLabel->clearPreview();
        isDrawingRect = false;
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

void leftPart::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (!currentPixmap.isNull())
    {
        updateImageDisplay();
    }
}

// 图像展示
void leftPart::displayImage(const QString &imagePath)
{
    QImage image(imagePath);
    if (image.isNull()) return;

    currentPixmap = QPixmap::fromImage(image);
    originalImageSize = currentPixmap.size();

    imageLabel->setOriginalImageSize(originalImageSize);
    imageLabel->setLabels(labels);
    imageLabel->setPixmap(currentPixmap);

    updateImageDisplay();
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

// 滚轮操作处更新图像显示
void leftPart::updateImageDisplay()
{
    if (currentPixmap.isNull()) return;

    // 计算可用区域
    QSize viewportSize = imageArea->viewport()->size();
    int scrollBarWidth = imageArea->verticalScrollBar()->sizeHint().width();
    int scrollBarHeight = imageArea->horizontalScrollBar()->sizeHint().height();
    QSize availableSize(viewportSize.width() - scrollBarWidth, viewportSize.height() - scrollBarHeight);

    // 计算缩放比例
    double ratio = qMin(static_cast<double>(availableSize.width() / originalImageSize.width()),
                        static_cast<double>(availableSize.height() / originalImageSize.height()));

    QSize scaledSize = isZoomed ? originalImageSize * scaleFactor : originalImageSize * ratio;

    // 创建缩放图像
    QPixmap scaledPixmap = currentPixmap.scaled(
        scaledSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    imageLabel->setPixmap(scaledPixmap);
    imageLabel->resize(scaledSize);
}

// 鼠标事件重写
// 重写鼠标滚轮事件
void leftPart::wheelEvent(QWheelEvent *event)
{
    if (currentPixmap.isNull())
    {
        event->ignore();
        return;
    }

    // 获取当前鼠标位置(相对于scrollArea)
    QPointF currentPos = event->pos();
    QPointF imageAreaPos = imageArea->mapFromParent(currentPos.toPoint());

    if (!imageArea->rect().contains(currentPos.toPoint()))
    {
        event->ignore();
        return;
    }

    // 计算鼠标在原始图像上的相对位置
    double imageX = (imageAreaPos.x() + imageArea->horizontalScrollBar()->value() - (imageArea->width() - imageLabel->width()) / 2) / scaleFactor;
    double imageY = (imageAreaPos.y() + imageArea->verticalScrollBar()->value() - (imageArea->height() - imageLabel->height()) / 2) / scaleFactor;
    imageX = qBound(0.0, imageX, static_cast<double>(originalImageSize.width()));
    imageY = qBound(0.0, imageY, static_cast<double>(originalImageSize.height()));

     // ctrl + 鼠标滚轮    缩放
     if (event->modifiers() & Qt::ControlModifier)
     {
         // 针对初次缩放计算当前窗口的缩放比例
         if (!isZoomed)
         {
             double widthRatio = static_cast<double>(imageLabel->width()) / originalImageSize.width();
             double heightRatio = static_cast<double>(imageLabel->height()) / originalImageSize.height();
             scaleFactor = qMin(widthRatio, heightRatio);
             isZoomed = true;
         }

        const double scaleStep = 0.1;
        double oldScale = scaleFactor;

        // 计算缩放因子
        double angle = event->angleDelta().y();
        if (angle != 0)
        {
            double factor = (angle > 0) ? (1.0 + scaleStep) : (1.0 / (1.0 + scaleStep));
            double newScale = oldScale * factor;
            newScale = qMax(0.5, qMin(newScale, 10.0));

            // 新的鼠标位置
            double newImageX = imageX * newScale;
            double newImageY = imageY * newScale;

            // 新的滚动条位置
            double newScrollX = newImageX - currentPos.x() + (imageArea->width() - imageLabel->width()) / 2;
            double newScrollY = newImageY - currentPos.y() + (imageArea->height() - imageLabel->height()) / 2;

            // 设置滚动条位置
            imageArea->horizontalScrollBar()->setValue(newScrollX);
            imageArea->verticalScrollBar()->setValue(newScrollY);

            scaleFactor = newScale;
            isZoomed = true;

            updateImageDisplay();
        }
        event->accept();
    }
    // alt + 鼠标滚轮    左右移动
    else if (event->modifiers() & Qt::AltModifier)
    {
         int delta = event->angleDelta().y();
         if (delta != 0)
         {
             int scrollStep = 20;
             int dx = (delta > 0) ? -scrollStep : scrollStep;

             // 限制滚动条在有效范围内
             QScrollBar *xScroll = imageArea->horizontalScrollBar();
             int newValue = xScroll->value() + dx;
             newValue = qMax(xScroll->minimum(), qMin(newValue, xScroll->maximum()));
             imageArea->horizontalScrollBar()->setValue(newValue);
         }
         event->accept();
    }
    // 仅鼠标滚轮    上下移动
    else
    {
         int delta = event->angleDelta().y();
         if (delta != 0)
         {
             int scrollStep = 20;
             int dy = (delta > 0) ? -scrollStep : scrollStep;

             QScrollBar *yScroll = imageArea->verticalScrollBar();
             int newValue = yScroll->value() + dy;
             newValue = qMax(yScroll->minimum(), qMin(newValue, yScroll->maximum()));
             imageArea->verticalScrollBar()->setValue(newValue);
         }
         event->accept();
    }
}

// 重写鼠标左键事件
void leftPart::mousePressEvent(QMouseEvent *event)
{
    if (is_labeling && !currentPixmap.isNull() && event->button() == Qt::LeftButton)
    {
        QPoint imgPos = convertToImageCoordinates(event->pos());
        if (imgPos.isNull()) return;

        if (firstPoint.isNull())
        {
            firstPoint = imgPos;
            imageLabel->setFirstPoint(firstPoint);
            isDrawingRect = true;
            imageLabel->setDrawingState(true);
            emit statusMessageUpdate("标签模式: 点击图像设置终点");
        }
    }
    else
    {
        // 其他事件仍由父类处理
        QWidget::mousePressEvent(event);
    }
}

// 重写鼠标释放事件
void leftPart::mouseReleaseEvent(QMouseEvent *event)
{
    if (is_labeling && isDrawingRect && event->button() == Qt::LeftButton && !firstPoint.isNull() && !currentPixmap.isNull())
    {
        QPoint imgPos = convertToImageCoordinates(event->pos());
        if (!imgPos.isNull())
        {
            QRect rect(firstPoint, imgPos);
            rect = rect.normalized();

            if (rect.width() > 0 && rect.height() > 0)
            {
                labels.push_back(rect);
                emit statusMessageUpdate("标签添加成功");
                imageLabel->setLabels(labels);
            }
            else
            {
                emit statusMessageUpdate("标签添加失败：高度或宽度为0");
            }

            isDrawingRect = false;
            imageLabel->setDrawingState(false);
            firstPoint = QPoint();
            imageLabel->clearFirstPoint();
            imageLabel->clearPreview();

            if (is_labeling)
            {
                emit statusMessageUpdate("标签模式: 点击图像设置起点 (ESC取消)");
            }
        }
    }
    else
    {
         QWidget::mouseReleaseEvent(event);
    }
}

// 重写鼠标移动事件
void leftPart::mouseMoveEvent(QMouseEvent *event)
{
    if (is_labeling && isDrawingRect && !firstPoint.isNull() && !currentPixmap.isNull())
    {
        currentPoint = convertToImageCoordinates(event->pos());

        if (!currentPoint.isNull())
        {
            QRect previewRect(firstPoint, currentPoint);
            previewRect = previewRect.normalized();
            imageLabel->setPreviewRect(previewRect);
        }
    }
    else
    {
        QWidget::mouseMoveEvent(event);
    }
}

// 取消绘制
void leftPart::cancelDrawing()
{
    if (isDrawingRect)
    {
        isDrawingRect = false;
        imageLabel->setDrawingState(false);
        imageLabel->clearFirstPoint();
        imageLabel->clearPreview();

        if (is_labeling)
        {
            emit statusMessageUpdate("标签模式: 绘制已取消");
        }
    }
}

// 键盘事件重写
void leftPart::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && isDrawingRect)
    {
        cancelDrawing();
        event->accept();
    }
    else
    {
        QWidget::keyPressEvent(event);
    }
}

// 将窗口坐标转换为图像坐标
QPoint leftPart::convertToImageCoordinates(const QPoint &pos)
{
    QPoint viewportPos = imageArea->mapFrom(this, pos);

    // 滚动条的偏移
    int xInLabel = viewportPos.x() + imageArea->horizontalScrollBar()->value();
    int yInLabel = viewportPos.y() + imageArea->verticalScrollBar()->value();

    const QPixmap* pixmap = imageLabel->pixmap();
    if (!pixmap || pixmap->isNull())
        return QPoint();

    QSize pixmapSize = pixmap->size();
    QSize labelSize = imageLabel->size();

    // 计算图像在标签中的偏移（居中显示）
    int xOffset = (labelSize.width() - pixmapSize.width()) / 2;
    int yOffset = (labelSize.height() - pixmapSize.height()) / 2;

    int imgX = xInLabel - xOffset;
    int imgY = yInLabel - yOffset;

    imgX = qBound(0, imgX, pixmapSize.width() - 1);
    imgY = qBound(0, imgY, pixmapSize.height() - 1);

    // 转换为原始图像坐标
    double scaleX = static_cast<double>(originalImageSize.width()) / pixmapSize.width();
    double scaleY = static_cast<double>(originalImageSize.height()) / pixmapSize.height();

    return QPoint(
        static_cast<int>(imgX * scaleX),
        static_cast<int>(imgY * scaleY)
    );
}

// 缩放矩形到当前显示尺寸
QRect leftPart::scaleRect(const QRect &rect)
{
    // 获取图像标签中的图像
    const QPixmap* pixmap = imageLabel->pixmap();
    if (!pixmap || pixmap->isNull())
    {
        return QRect();
    }

    QSize pixmapSize = pixmap->size();

    // 计算缩放比例
    double scaleX = static_cast<double>(pixmapSize.width()) / originalImageSize.width();
    double scaleY = static_cast<double>(pixmapSize.height()) / originalImageSize.height();

    // 缩放矩形
    return QRect(
        static_cast<int>(rect.x() * scaleX),
        static_cast<int>(rect.y() * scaleY),
        static_cast<int>(rect.width() * scaleX),
        static_cast<int>(rect.height() * scaleY)
    );
}

