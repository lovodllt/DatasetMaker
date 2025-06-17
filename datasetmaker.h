#ifndef DATASETMAKER_H
#define DATASETMAKER_H

#include <QMainWindow>
#include <QSplitter>
#include <QLabel>
#include <QVBoxLayout>
#include <QString>
#include <QStatusBar>
#include <QAction>
#include <QFileDialog>
#include <QApplication>

#include "cls.h"
#include "detection.h"
#include "left.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DatasetMaker; }
QT_END_NAMESPACE

class DatasetMaker : public QMainWindow
{
    Q_OBJECT

public:
    DatasetMaker(QWidget *parent = nullptr);
    ~DatasetMaker();

private slots:
    void on_openFile_triggered();
    void on_cls_triggered();
    void on_detection_triggered();

private:
    void selectAndProcessDirectory();
    void processDirectory(const QString& dirPath);
    void changeWidget(int targetIndex, QWidget *newWidget);
    void handleLeftButton(int index);

    Ui::DatasetMaker *ui;
    QSplitter *splitter;                       // 上下分割器
    QSplitter *horizontalSplitter;             // 左右分割器
    QWidget *placeholderWidget;                // 占位Widget

    QFileInfoList imageFiles;                  // 存储图片信息
    QMap<QString, bool> is_images_processed;

    cls *clsWidget;
    detection *detectionWidget;
    left *leftWidget;
};
#endif // DATASETMAKER_H






