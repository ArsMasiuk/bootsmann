#ifndef CMAINGUI_H
#define CMAINGUI_H

#include <QWidget>

namespace Ui {
class CMainGUI;
}

class CMainGUI : public QWidget
{
    Q_OBJECT

public:
    explicit CMainGUI(QWidget *parent = nullptr);
    ~CMainGUI();

private:
    Ui::CMainGUI *ui;
};

#endif // CMAINGUI_H
