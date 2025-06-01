#ifndef CREQUESTGUI_H
#define CREQUESTGUI_H

#include <QWidget>
#include <QNetworkReply>
#include <QNetworkCacheMetaData>
#include <QElapsedTimer>
#include <QSettings>

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

	void Store(QSettings& settings) const;
    void Restore(const QSettings& settings);

Q_SIGNALS:
    void RequestTitleChanged(const QString& title);

public Q_SLOTS:
    void OnRequestSuccess();
	void OnRequestError(QNetworkReply::NetworkError code);

private Q_SLOTS:
    void on_Run_clicked();

	void on_AddHeader_clicked();
    void on_RemoveHeader_clicked();
	void on_ClearHeaders_clicked();

	void on_AddParameter_clicked();
	void on_RemoveParameter_clicked();
	void on_ClearParameters_clicked();

private:
    void SetDefaultHeaders();
	void AddRequestHeader(const QString& name, const QString& value);
	void AddRequestHeader(QNetworkRequest::KnownHeaders type, const QString& value);
    QNetworkCacheMetaData::RawHeaderList GetRequestHeaders() const;

    void LockRequest();
	void UnlockRequest();
	void ClearResult();

    Ui::CRequestGUI *ui;

    CRequestManager& m_reqMgr;
	QElapsedTimer m_timer;
};

#endif // CREQUESTGUI_H
