#include "left.h"
#include "ui_left.h"

left::left(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::left)
{
    ui->setupUi(this);

    // 创建主水平布局
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    setLayout(mainLayout);

    // 左侧垂直布局
    QVBoxLayout *leftLayout = new QVBoxLayout();

    // 按钮区域
    leftLayout->addWidget(ui->openDir);
    leftLayout->addWidget(ui->nextImage);
    leftLayout->addWidget(ui->prev_Image);

    // 创建图像文件列表的QListWidget
    imageListWidget = new QListWidget(this);
    leftLayout->addWidget(imageListWidget);

    // 创建显示图像的QLabel
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 布局调整
    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addWidget(imageLabel, 5);

    connect(imageListWidget, &QListWidget::itemClicked, this, &left::on_imageListWidget_itemClicked);
}

left::~left()
{
    delete ui;
}

void left::on_openDir_clicked()
{
    emit buttonClicked(1);
}

void left::on_nextImage_clicked()
{
    emit buttonClicked(2);
}

void left::on_prev_Image_clicked()
{
    emit buttonClicked(3);
}

// 图像展示
void left::displayImage(const QString &imagePath)
{
    QImage image(imagePath);
    if (!image.isNull())
    {
        currentPixmap = QPixmap::fromImage(image);
        originalImageSize = currentPixmap.size();

        scaleFactor = 1.0;
        scrollOffset = QPointF(0, 0);
        isZoomed = false;

        onWindowResized(size());
    }
}

// 创建图像文件条目
void left::populateImageList(const QFileInfoList &imageFiles, QMap<QString, bool> &is_images_processed)
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
void left::on_imageListWidget_itemClicked(QListWidgetItem *item)
{
    int index = imageListWidget->row(item);
    if (index >= 0 && index < imageFiles.size())
    {
        displayImage(imageFiles[index].absoluteFilePath());
    }
}

// 更据imageLabel的可用大小重新缩放currentPixmap
void left::onWindowResized(const QSize &size)
{
    if (currentPixmap.isNull()) return;

    // 保存当前显示区域大小
    QSize availableSize = imageLabel->size();
    lastDisplaySize = availableSize;

    // 计算缩放后图像大小
    QSize scaledSize;

    if (isZoomed)
    {
        scaledSize = QSize(originalImageSize.width() * scaleFactor, originalImageSize.height() * scaleFactor);
        // 限制滚动偏移量
        scrollOffset.setX(qMax(0.0, qMin(scrollOffset.x(), static_cast<qreal>(scaledSize.width() - availableSize.width()))));
        scrollOffset.setY(qMax(0.0, qMin(scrollOffset.y(), static_cast<qreal>(scaledSize.height() - availableSize.height()))));
    }
    else
    {
        double widthRatio = static_cast<double>(availableSize.width() / originalImageSize.width());
        double heightRatio = static_cast<double>(availableSize.height() / originalImageSize.height());
        double ratio = qMin(widthRatio, heightRatio);
        scaledSize = originalImageSize * ratio;
    }

    // 创建缩放后图像
    QPixmap scaledPixmap = currentPixmap.scaled(
       scaledSize,
       Qt::KeepAspectRatio,
       Qt::SmoothTransformation
    );

    imageLabel->setPixmap(scaledPixmap);
}

// 重写鼠标滚轮事件
void left::wheelEvent(QWheelEvent *event)
{
    if (currentPixmap.isNull())
    {
        event->ignore();
        return;
    }

    // 获取当前鼠标位置(相对于imageLabel)
    QPointF labelPos = imageLabel->mapFrom(this, event->pos());

    // 计算鼠标在原始图像上的相对位置
    double imageX = (labelPos.x() + scrollOffset.x() - (lastDisplaySize.width() - originalImageSize.width() * scaleFactor) / 2) / scaleFactor;
    double imageY = (labelPos.y() + scrollOffset.y() - (lastDisplaySize.height() - originalImageSize.height() * scaleFactor) / 2) / scaleFactor;
    imageX = qBound(0.0, imageX, static_cast<double>(originalImageSize.width()));
    imageY = qBound(0.0, imageY, static_cast<double>(originalImageSize.height()));

    if (imageLabel->rect().contains(labelPos.toPoint()))
    {
        // ctrl + 鼠标滚轮    缩放
        if (event->modifiers() & Qt::ControlModifier)
        {
            const double scaleStep = 0.1;
            double oldScale = scaleFactor;

            // 计算缩放因子
            double angle = event->angleDelta().y();
            if (angle != 0)
            {
                double factor = (angle > 0) ? (1.0 + scaleStep) : (1.0 / (1.0 + scaleStep));
                double newScale = oldScale * factor;
                newScale = qMax(0.5, qMin(newScale, 5.0));

                QPointF oldCenter = labelPos + scrollOffset;
                scaleFactor = newScale;
                QPointF newCenter = oldCenter * (newScale / oldScale);
                scrollOffset = newCenter - labelPos;

                isZoomed = true;
            }
        }
        // alt + 鼠标滚轮    左右移动
        else if (event->modifiers() & Qt::AltModifier)
        {
            double scaledWidth = originalImageSize.width() * scaleFactor;
            if (scaledWidth > lastDisplaySize.width())
            {
                const int scrollStep = 20;
                int dx = event->angleDelta().y() > 0 ? -scrollStep : scrollStep;
                scrollOffset.setX(scrollOffset.x() + dx);
                isZoomed = true;
            }
        }
        // 仅鼠标滚轮    上下移动
        else
        {
            double scaledHeight = originalImageSize.height() * scaleFactor;
            if (scaledHeight > lastDisplaySize.height())
            {
                const int scrollStep = 20;
                int dy = event->angleDelta().y() > 0 ? -scrollStep :scrollStep;
                scrollOffset.setY(scrollOffset.y() + dy);
                isZoomed = true;
            }
        }

        onWindowResized(lastDisplaySize);
        event->accept();
        return;
    }

    event->ignore();
}






