#include "datasetmaker.h"
#include "ui_datasetmaker.h"

DatasetMaker::DatasetMaker(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DatasetMaker)
{
    ui->setupUi(this);

    // 创建垂直分割器
    splitter = new QSplitter(Qt::Vertical, this);
    setCentralWidget(splitter);

    // 创建上方固定组件
    QWidget *topWidget = new QWidget(this);
    QVBoxLayout *topLayout = new QVBoxLayout(topWidget);

    splitter->addWidget(topWidget);

    // 创建水平分割器用于左边和右边
    horizontalSplitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(horizontalSplitter);

    // 将leftWidget加入左方组件并连接信号
    leftWidget = new left(this);
    horizontalSplitter->addWidget(leftWidget);
    connect(leftWidget, &left::buttonClicked, this, &DatasetMaker::handleLeftButton);

    // 创建占位Widget
    placeholderWidget = new QWidget(this);
    QLabel *hintLabel = new QLabel("请选择模式", placeholderWidget);
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QVBoxLayout *layout = new QVBoxLayout(placeholderWidget);
    layout->addWidget(hintLabel);
    layout->addStretch(1);

    horizontalSplitter->addWidget(placeholderWidget);

    clsWidget = new cls(this);
    detectionWidget = new detection(this);
    clsWidget->hide();
    detectionWidget->hide();

    splitter->setSizes({50, 600});
    horizontalSplitter->setSizes({400, 200});

    // 分割线
    splitter->setStyleSheet(R"(
        QSplitter::handle {
            background-color: #888;    /* 分割线颜色 */
            width: 5px;                /* 水平分割线宽度（垂直分割时用 height） */
            margin: 0 2px;             /* 边距，让分割线居中 */
        }
    )");

    horizontalSplitter->setStyleSheet(R"(
        QSplitter::handle {
            background-color: #888;    /* 分割线颜色 */
            width: 5px;                /* 水平分割线宽度（垂直分割时用 height） */
            margin: 0 2px;             /* 边距，让分割线居中 */
        }
    )");
}

DatasetMaker::~DatasetMaker()
{
    delete ui;
}

// 文件选择和处理
void DatasetMaker::selectAndProcessDirectory()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        tr("选择文件夹"),
        "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (!dirPath.isEmpty())
    {
        ui->statusBar->showMessage(tr("已选择文件夹: %1").arg(dirPath));
        processDirectory(dirPath);
    }
}

// 文件选择和处理
void DatasetMaker::processDirectory(const QString& dirPath)
{
    QDir dir(dirPath);
    QStringList filters;   // 文件过滤器
    filters << "*.png" << "*.jpg" << "*.jpeg";
    imageFiles = dir.entryInfoList(filters, QDir::Files);

    is_images_processed.clear();

    for (const QFileInfo &fileInfo : imageFiles)
    {
        is_images_processed[fileInfo.absoluteFilePath()] = false;
    }

    leftWidget->populateImageList(imageFiles, is_images_processed);
}

// 删除非占位界面
void DatasetMaker::changeWidget(int targetIndex, QWidget *newWidget)
{
    // 获取水平分割器
    QSplitter *horizontalSplitter = qobject_cast<QSplitter*>(splitter->widget(1));
    if (!horizontalSplitter) return;

    if (targetIndex < 0 || targetIndex >= horizontalSplitter->count())
    {
        return;
    }

    QWidget *oldWidget = horizontalSplitter->widget(targetIndex);
    horizontalSplitter->replaceWidget(targetIndex, newWidget);

    oldWidget->hide();

    newWidget->show();
}

// 菜单栏打开文件选择
void DatasetMaker::on_openFile_triggered()
{
    selectAndProcessDirectory();
}

// 菜单栏模式选择
void DatasetMaker::on_cls_triggered()
{
    int targetIndex = 1;

    changeWidget(targetIndex, clsWidget);

    ui->statusBar->showMessage("切换到图像分类模式");
}

// 菜单栏模式选择
void DatasetMaker::on_detection_triggered()
{
    int targetIndex = 1;

    changeWidget(targetIndex, detectionWidget);

    ui->statusBar->showMessage("切换到装甲板目标检测模式");
}

// 左侧界面按钮处理
void DatasetMaker::handleLeftButton(int index)
{
    switch (index)
    {
        case 1:
            selectAndProcessDirectory();
            break;
        default:
            break;
    }
}



