#ifndef DATASETMAKER_H
#define DATASETMAKER_H
#pragma once

#include <QMainWindow>
#include <QMap>
#include <QSettings>

#include "cls.h"
#include "detection.h"
#include "leftPart.h"

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
    void on_yolo_triggered();
    void on_exit_triggered();
    void on_clsMode_triggered();
    void on_detectionMode_triggered();
    void statusMessageUpdate(const QString &message);

private:
    Ui::DatasetMaker *ui;

    leftPart *leftWidget() const;
    cls *clsWidget() const;
    detection *detectionWidget() const;

};

#endif // DATASETMAKER_H






