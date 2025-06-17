#ifndef CLS_H
#define CLS_H

#include <QWidget>

namespace Ui { class cls; }

class cls : public QWidget
{
    Q_OBJECT

public:
    explicit cls(QWidget *parent = nullptr);
    ~cls();

private slots:
    void on_one_clicked(bool checked);

private:
    Ui::cls *ui;
};

#endif // CLS_H
