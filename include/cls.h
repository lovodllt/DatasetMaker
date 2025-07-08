#ifndef CLS_H
#define CLS_H
#pragma once

#include <QAbstractButton>
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

    double getConfidenceThreshold() const;
    QString getModelSelection() const;

public slots:
    void onClassSelected(QAbstractButton *button);
    void on_sure_clicked();
    void on_createLabel_clicked();
    void on_autoMode_toggled(bool checked);
    void on_warp_toggled(bool checked);
    void on_confidenceEdit_textChanged(const QString &text);
    void on_nmsEdit_textChanged(const QString &text);
    void on_modelSelection_currentTextChanged(const QString &text);

    void displayPreview();
    void onLabelSelected(detectionLabel &label);

signals:
    void statusMessageUpdate(const QString &message);

private:
    Ui::cls *ui;
    QLabel *previewLabel;
    leftPart *leftPartInstance;
    autoMode *autoModeInstance;

    QButtonGroup *classButtonGroup;

public:
    std::string clsname{};
    int width{};
    int height{};
    cv::Mat label_img;
};

#endif // CLS_H
