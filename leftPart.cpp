#include "leftPart.h"
#include "ui_leftPart.h"
#include <QSplitter>

leftPart::leftPart(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::leftPart)
{
    ui->setupUi(this);

    // 创建主水平布局
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0); // 移除边距
    setLayout(mainLayout);

    // 创建外层分割器（水平分割按钮区域和图像区域）
    QSplitter *outerSplitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(outerSplitter);

    // 创建内部垂直分割器（用于按钮和图像列表）
    QSplitter *innerSplitter = new QSplitter(Qt::Vertical, this);
    outerSplitter->addWidget(innerSplitter);

    // 按钮区域
    QWidget *buttonWidget = new QWidget(outerSplitter);
    QVBoxLayout *buttonLayout = new QVBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(5, 5, 5, 5);                  // 添加内边距
    buttonLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);   // 顶部对齐并水平居中

    // 设置按钮为正方形并居中
    int buttonSize = 70;
    ui->openDir->setFixedSize(buttonSize, buttonSize);
    ui->nextImage->setFixedSize(buttonSize, buttonSize);
    ui->prevImage->setFixedSize(buttonSize, buttonSize);
    ui->createLabel->setFixedSize(buttonSize, buttonSize);
    ui->save->setFixedSize(buttonSize, buttonSize);
    ui->deleteFile->setFixedSize(buttonSize, buttonSize);

    // 设置按钮之间的间距
    buttonLayout->setSpacing(10); // 增加按钮之间的垂直间距

    // 按钮从上到下排列，水平居中
    buttonLayout->addWidget(ui->openDir, 0, Qt::AlignHCenter);
    buttonLayout->addWidget(ui->nextImage, 0, Qt::AlignHCenter);
    buttonLayout->addWidget(ui->prevImage, 0, Qt::AlignHCenter);
    buttonLayout->addWidget(ui->createLabel, 0, Qt::AlignHCenter);
    buttonLayout->addWidget(ui->save, 0, Qt::AlignHCenter);
    buttonLayout->addWidget(ui->deleteFile, 0, Qt::AlignHCenter);
    buttonLayout->addStretch();

    // 图像列表区域
    QWidget *listWidget = new QWidget(innerSplitter);
    QVBoxLayout *listLayout = new QVBoxLayout(listWidget);
    listLayout->setContentsMargins(5, 5, 5, 5);

    // 创建图像列表
    imageListWidget = new QListWidget(listWidget);
    imageListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    listLayout->addWidget(imageListWidget);

    // 添加到分割器
    innerSplitter->addWidget(buttonWidget);
    innerSplitter->addWidget(listWidget);

    // 创建滚动区域（用于显示图像）
    scrollArea = new QScrollArea(outerSplitter);
    scrollArea->setWidgetResizable(true);
    scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 创建图像区域
    scrollArea->setWidgetResizable(true);
    scrollArea->setAlignment(Qt::AlignCenter);

    imageLabel = new QLabel(scrollArea);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(false);

    scrollArea->setWidget(imageLabel);
    outerSplitter->addWidget(scrollArea);

    // 设置分割器初始大小
    innerSplitter->setSizes({300, 100});
    outerSplitter->setSizes({50, 500});

    // 设置分割器样式
    outerSplitter->setStyleSheet(R"(
        QSplitter::handle {
            background-color: #cccccc;
            width: 5px;
            border-radius: 2px;
        }
    )");

    // 连接列表点击信号
    connect(imageListWidget, &QListWidget::itemClicked, this, &leftPart::on_imageListWidget_itemClicked);
}

leftPart::~leftPart()
{
    delete ui;
}

void leftPart::on_openDir_clicked()
{
    emit buttonClicked(1);
}

void leftPart::on_nextImage_clicked()
{
    emit buttonClicked(2);
}

void leftPart::on_prevImage_clicked()
{
    emit buttonClicked(3);
}

void leftPart::on_createLabel_clicked()
{
    emit buttonClicked(4);
}
void leftPart::on_save_clicked()
{
    emit buttonClicked(5);
}

void leftPart::on_deleteFile_clicked()
{
    emit buttonClicked(6);
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
    if (!image.isNull())
    {
        currentPixmap = QPixmap::fromImage(image);
        originalImageSize = currentPixmap.size();

        scaleFactor = 1.0;
        isZoomed = false;

        updateImageDisplay();
    }
}

// 创建图像文件条目
void leftPart::populateImageList(const QFileInfoList &imageFiles, QMap<QString, bool> &is_images_processed)
{
    this->imageFiles = imageFiles;
    this->is_images_processed = is_images_processed;

    imageListWidget->clear();

    for (const QFileInfo &fileInfo : imageFiles)
    {
        QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        imageListWidget->addItem(item);
    }

    if (!imageFiles.isEmpty())
    {
        displayImage(imageFiles.first().absoluteFilePath());
    }
}

// 图像文件条目点击处理
void leftPart::on_imageListWidget_itemClicked(QListWidgetItem *item)
{
    int index = imageListWidget->row(item);
    if (index >= 0 && index < imageFiles.size())
    {
        displayImage(imageFiles[index].absoluteFilePath());
    }
}

// 更新图像显示
void leftPart::updateImageDisplay()
{
    if (currentPixmap.isNull())
        return;

    // 计算缩放图像大小
    QSize scaledSize;
    if (isZoomed)
    {
        scaledSize = originalImageSize * scaleFactor;
    }
    else
    {
        QSize availableSize = scrollArea->viewport()->size();
        double widthRatio = static_cast<double>(availableSize.width()) / originalImageSize.width();
        double heightRatio = static_cast<double>(availableSize.height()) / originalImageSize.height();
        double ratio = qMin(widthRatio, heightRatio);
        scaledSize = originalImageSize * ratio;
    }

    // 创建缩放图像
    QPixmap scaledPixmap = currentPixmap.scaled(
        scaledSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    imageLabel->setPixmap(scaledPixmap);
    imageLabel->resize(scaledSize);
}

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
    QPointF scrollAreaPos = scrollArea->mapFromParent(currentPos.toPoint());

    if (!scrollArea->rect().contains(currentPos.toPoint()))
    {
        event->ignore();
        return;
    }

    // 计算鼠标在原始图像上的相对位置
    double imageX = (scrollAreaPos.x() + scrollArea->horizontalScrollBar()->value() - (scrollArea->width() - imageLabel->width()) / 2) / scaleFactor;
    double imageY = (scrollAreaPos.y() + scrollArea->verticalScrollBar()->value() - (scrollArea->height() - imageLabel->height()) / 2) / scaleFactor;
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
            newScale = qMax(0.1, qMin(newScale, 10.0));

            // 新的鼠标位置
            double newImageX = imageX * newScale;
            double newImageY = imageY * newScale;

            // 新的滚动条位置
            double newScrollX = newImageX - currentPos.x() + (scrollArea->width() - imageLabel->width()) / 2;
            double newScrollY = newImageY - currentPos.y() + (scrollArea->height() - imageLabel->height()) / 2;

            // 设置滚动条位置
            scrollArea->horizontalScrollBar()->setValue(newScrollX);
            scrollArea->verticalScrollBar()->setValue(newScrollY);

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
             QScrollBar *xScroll = scrollArea->horizontalScrollBar();
             int newValue = xScroll->value() + dx;
             newValue = qMax(xScroll->minimum(), qMin(newValue, xScroll->maximum()));
             scrollArea->horizontalScrollBar()->setValue(newValue);
         }
         event->accept();
    }
    // 仅鼠标滚轮    上下移动
    else
    {
         if (!isZoomed)
         {
             isZoomed = true;
             updateImageDisplay();
         }

         int delta = event->angleDelta().y();
         if (delta != 0)
         {
             int scrollStep = 20;
             int dy = (delta > 0) ? -scrollStep : scrollStep;

             QScrollBar *yScroll = scrollArea->verticalScrollBar();
             int newValue = yScroll->value() + dy;
             newValue = qMax(yScroll->minimum(), qMin(newValue, yScroll->maximum()));
             scrollArea->verticalScrollBar()->setValue(newValue);
         }
         event->accept();
    }
}
