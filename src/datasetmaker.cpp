#include "datasetmaker.h"
#include "ui_datasetmaker.h"

DatasetMaker::DatasetMaker(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DatasetMaker)
{
    ui->setupUi(this);

    // 初始状态
    ui->rightStack->setCurrentIndex(0);
    ui->statusBar->showMessage("就绪");

    // 传递指针
    clsWidget()->setLeftPart(leftWidget());
    detectionWidget()->setLeftPart(leftWidget());

    connect(leftWidget(), &leftPart::statusMessageUpdate, this, &DatasetMaker::statusMessageUpdate);
    connect(clsWidget(), &cls::statusMessageUpdate, this, &DatasetMaker::statusMessageUpdate);
    connect(leftWidget()->autoModeInstance.get(), &autoMode::statusMessageUpdate, this, &DatasetMaker::statusMessageUpdate);
    connect(leftWidget()->imageLabel, &ImageLabel::previewRequested, clsWidget(), &cls::displayPreview);
    connect(detectionWidget(), &detection::statusMessageUpdate, this, &DatasetMaker::statusMessageUpdate);
}

DatasetMaker::~DatasetMaker()
{
    delete ui;
}

// 实现成员函数
leftPart *DatasetMaker::leftWidget() const{
    return ui->leftContainer;
}

cls *DatasetMaker::clsWidget() const {
    return qobject_cast<cls*>(ui->rightStack->widget(1));
}

detection *DatasetMaker::detectionWidget() const {
    return qobject_cast<detection*>(ui->rightStack->widget(2));
}

// 菜单栏打开文件选择
void DatasetMaker::on_openFile_triggered()
{
    leftWidget()->openFile();
}

void DatasetMaker::on_selectSavePath_triggered()
{
    savePath_ = "";
    leftWidget()->saveFilePath();
}

// 文件保存格式
void DatasetMaker::on_yolo_triggered()
{
    saveFormat_ = "yolo";
    leftWidget()->openFile();
}

void DatasetMaker::on_exit_triggered()
{
    QSettings settings("MyCompany", "DatasetMaker");

    // 保存窗口布局
    settings.setValue("windowGeometry", saveGeometry());
    settings.setValue("windowState", saveState());

    // 退出应用程序
    QApplication::quit();
}

// 菜单栏模式选择
void DatasetMaker::on_clsMode_triggered()
{
    labelMode_ = "cls";

    detectionLabels_.clear();

    if (leftWidget() && !leftWidget()->imageLabel->originalImg.empty())
    {
        leftWidget()->imageLabel->tmpLabel = detectionLabel();
        leftWidget()->imageLabel->drawLabels();
        leftWidget()->imageLabel->clearLabels();

        is_warp_ = false;
        is_binary_ = false;
        is_poseMode_ = false;
    }

    ui->rightStack->setCurrentWidget(clsWidget());
    ui->statusBar->showMessage("切换到图像分类模式");
}

// 菜单栏模式选择
void DatasetMaker::on_detectionMode_triggered()
{
    labelMode_ = "detection";

    detectionLabels_.clear();

    if (leftWidget() && !leftWidget()->imageLabel->originalImg.empty())
    {
        leftWidget()->imageLabel->tmpLabel = detectionLabel();
        leftWidget()->imageLabel->drawLabels();
        leftWidget()->imageLabel->clearLabels();

        is_warp_ = false;
        is_binary_ = false;
        is_poseMode_ = false;
    }

    ui->rightStack->setCurrentWidget(detectionWidget());
    ui->statusBar->showMessage("切换到装甲板目标检测模式");
}

// 菜单栏快捷键
void DatasetMaker::on_shortCut_triggered()
{

}

// 状态栏更新
void DatasetMaker::statusMessageUpdate(const QString &message)
{
    ui->statusBar->showMessage(message);
}
