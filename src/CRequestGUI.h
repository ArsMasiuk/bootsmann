#ifndef CREQUESTGUI_H
#define CREQUESTGUI_H

#include <QWidget>
#include <QNetworkReply>
#include <QElapsedTimer>

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

Q_SIGNALS:
    void RequestTitleChanged(const QString& title);

public Q_SLOTS:
    void OnRequestSuccess();
	void OnRequestError(QNetworkReply::NetworkError code);

private Q_SLOTS:
    void on_Run_clicked();

private:
    Ui::CRequestGUI *ui;

    CRequestManager& m_reqMgr;
	QElapsedTimer m_timer;
};

#endif // CREQUESTGUI_H
