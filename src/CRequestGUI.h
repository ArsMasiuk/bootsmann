#ifndef CREQUESTGUI_H
#define CREQUESTGUI_H

#include <QWidget>

namespace Ui {
class CRequestGUI;
}

class CRequestManager;

class CRequestGUI : public QWidget
{
    Q_OBJECT

public:
    explicit CRequestGUI(CRequestManager& reqMgr, QWidget *parent = nullptr);
    ~CRequestGUI();

    void Init();

private Q_SLOTS:
    void on_Run_clicked();

private:
    Ui::CRequestGUI *ui;

    CRequestManager& m_reqMgr;
};

#endif // CREQUESTGUI_H
