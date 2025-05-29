#include "CRequestManager.h"

CRequestManager::CRequestManager(QObject *parent)
    : QObject{parent}
{

}


QNetworkReply* CRequestManager::SendRequest(QObject* handler, const QByteArray& verb, const QUrl& url, const QByteArray& payload,
    QNetworkCacheMetaData::RawHeaderList headers)
{
    QNetworkRequest request;
    request.setUrl(url);

    for (const auto& header : headers) {
        request.setRawHeader(header.first, header.second);
	}
    //request.setRawHeader("User-Agent", "Bootsmann 1.0");
    //request.setHeader(QNetworkRequest::UserAgentHeader, "Bootsmann 1.0");
    //request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (verb == "GET"){
        auto reply = m_manager.get(request);
        return DoProcessReply(handler, reply);
    }

    if (verb == "POST"){
        auto reply = m_manager.post(request, payload);
        return DoProcessReply(handler, reply);
    }

    if (verb == "PUT"){
        auto reply = m_manager.put(request, payload);
        return DoProcessReply(handler, reply);
    }

    if (verb == "DELETE"){
        auto reply = m_manager.deleteResource(request);
        return DoProcessReply(handler, reply);
    }

    if (verb == "HEAD"){
        auto reply = m_manager.head(request);
        return DoProcessReply(handler, reply);
    }

    // verb unknown, custom request instead
    auto reply = m_manager.sendCustomRequest(request, verb, payload);
    return DoProcessReply(handler, reply);
}


QNetworkReply* CRequestManager::DoProcessReply(QObject* handler, QNetworkReply* reply)
{
    if (!handler || !reply){
        return nullptr;
    }

    connect(reply, SIGNAL(readyRead()), handler, SLOT(OnRequestSuccess()));
    connect(reply, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), handler, SLOT(OnRequestError(QNetworkReply::NetworkError)));

    // temp reply ID
    return reply;
}

