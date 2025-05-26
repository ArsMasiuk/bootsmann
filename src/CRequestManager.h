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

    QNetworkReply* SendRequest(QObject* handler, const QByteArray& verb, const QUrl& url, const QByteArray& payload = "");

Q_SIGNALS:
    void RequestSuccess(QNetworkReply* reply, int code, const QString& errorMsg);
    void RequestError(QNetworkReply* reply, int code, const QString& errorMsg);

protected:
    QNetworkReply* DoProcessReply(QObject* handler, QNetworkReply* reply);

private:
    QNetworkAccessManager m_manager;
};

#endif // CREQUESTMANAGER_H
