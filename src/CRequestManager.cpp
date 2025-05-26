#include "CRequestManager.h"

CRequestManager::CRequestManager(QObject *parent)
    : QObject{parent}
{

}


int CRequestManager::SendRequest(const QByteArray& verb, const QUrl& url, const QByteArray& payload)
{
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "Bootsmann 1.0");

    if (verb == "GET"){
        auto reply = m_manager.get(request);
        return DoProcessReply(reply);
    }

    if (verb == "POST"){
        auto reply = m_manager.post(request, payload);
        return DoProcessReply(reply);
    }

    if (verb == "PUT"){
        auto reply = m_manager.put(request, payload);
        return DoProcessReply(reply);
    }

    if (verb == "DELETE"){
        auto reply = m_manager.deleteResource(request);
        return DoProcessReply(reply);
    }

    if (verb == "HEAD"){
        auto reply = m_manager.head(request);
        return DoProcessReply(reply);
    }

    // verb unknown, custom request instead
    auto reply = m_manager.sendCustomRequest(request, verb, payload);
    return DoProcessReply(reply);
}


int CRequestManager::DoProcessReply(QNetworkReply* reply)
{
    if (!reply){
        return -100;
    }

    connect(reply, &QIODevice::readyRead, this, &CRequestManager::OnRequestSuccess);
    connect(reply, &QNetworkReply::errorOccurred, this, &CRequestManager::OnRequestError);

    // temp reply ID
    return (quintptr) reply;
}


void CRequestManager::OnRequestSuccess()
{
	auto reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater(); // delete reply object after processing

	auto data = reply->readAll();

    auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    // emit signal with data and status code
	Q_EMIT RequestSuccess((quintptr)reply, statusCode, data);
}


void CRequestManager::OnRequestError(QNetworkReply::NetworkError code)
{
    auto reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater(); // delete reply object after processing

    // emit signal with error status code
    Q_EMIT RequestError((quintptr)reply, code, "");
}
