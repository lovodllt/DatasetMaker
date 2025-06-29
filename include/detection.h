#ifndef DETECTION_H
#define DETECTION_H
#pragma once

#include <QWidget>
#include "common.h"

namespace Ui { class detection; }

class detection : public QWidget
{
    Q_OBJECT

public:
    explicit detection(QWidget *parent = nullptr);
    ~detection();

private:
    Ui::detection *ui;
};

#endif // DETECTION_H
