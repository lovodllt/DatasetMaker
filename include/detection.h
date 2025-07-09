#ifndef DETECTION_H
#define DETECTION_H
#pragma once

#include "cls.h"
#include "leftPart.h"

namespace Ui { class detection; }

class detection : public QWidget
{
    Q_OBJECT

public:
    explicit detection(QWidget *parent = nullptr);
    ~detection();
    void setLeftPart(leftPart *leftPart);

    void updateLabelList();
    void saveDetectionLabels();

public slots:
    void on_autoMode_toggled(bool checked);
    void on_inferAgain_clicked();
    void on_confidenceEdit_textChanged(const QString &text);
    void on_nmsEdit_textChanged(const QString &text);
    void on_modelSelection_currentTextChanged(const QString &text);
    void on_colorSave_toggled(bool checked);
    void on_createLabel_clicked();

    void onColorSelected(QAbstractButton *button);
    void on_labelList_itemClicked(QListWidgetItem *item);
    void onDetectionLabelSelected(detectionLabel &label);

signals:
    void statusMessageUpdate(const QString &message);

private:
    Ui::detection *ui;
    leftPart *leftPartInstance;
    QButtonGroup *colorSelection;

    std::string currentColor{};
};

#endif // DETECTION_H
