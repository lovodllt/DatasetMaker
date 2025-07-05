#ifndef CLS_H
#define CLS_H
#pragma once

#include <QAbstractButton>
#include <QDebug>
#include "common.h"
#include "leftPart.h"

namespace Ui { class cls; }

class ImageLabel;

class cls : public QWidget
{
    Q_OBJECT

public:
    explicit cls(QWidget *parent = nullptr);
    ~cls();

    void setLeftPart(leftPart *leftPart);

    QString createFileName(const QString &originalFileName);
    void saveCroppedImage(const cv::Mat &crop, const QString &originalFileName, const QString &className);
    void saveClsLabels();

public slots:
    void onClassSelected(QAbstractButton *button);
    void on_sure_clicked();
    void on_createLabel_clicked();
    void displayPreview();

signals:
    void statusMessageUpdate(const QString &message);

private:
    Ui::cls *ui;
    QLabel *previewLabel;
    leftPart *leftPartInstance;

public:
    std::string clsname{};
    int width{};
    int height{};
};

#endif // CLS_H
