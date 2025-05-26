#ifndef CREQUESTMANAGER_H
#define CREQUESTMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class CRequestManager : public QObject
{
    Q_OBJECT

public:
    explicit CRequestManager(QObject *parent = nullptr);

    int SendRequest(const QByteArray& verb, const QUrl& url, const QByteArray& payload = "");

protected:
    int DoProcessReply(QNetworkReply* reply);

private Q_SLOTS:
    void OnRequestSuccess();
    void OnRequestError(QNetworkReply::NetworkError code);

private:
    QNetworkAccessManager m_manager;
};

#endif // CREQUESTMANAGER_H
