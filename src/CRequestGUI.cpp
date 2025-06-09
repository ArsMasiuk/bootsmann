#include "CRequestGUI.h"
#include "ui_CRequestGUI.h"

#include "CRequestManager.h"

#include <QMessageBox>
#include <QJsonDocument>
#include <QTemporaryFile>
#include <QImageReader>
#include <QBuffer>


CRequestGUI::CRequestGUI(CRequestManager& reqMgr, QWidget *parent)
    : QWidget(parent)
    , m_reqMgr(reqMgr)
    , ui(new Ui::CRequestGUI)
{
    ui->setupUi(this);

    ui->splitter->setSizes(QList<int>({ INT_MAX, INT_MAX }));

	SetDefaultHeaders();

    ui->RequestTabs->setCurrentIndex(0);

	ui->ResponseHeaders->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    m_requestHL = new QSourceHighlite::QSourceHighliter(nullptr);
    m_replyHL = new QSourceHighlite::QSourceHighliter(nullptr);
}


CRequestGUI::~CRequestGUI()
{
    delete ui;

    delete m_requestHL;
    delete m_replyHL;
}


void CRequestGUI::Init()
{
    ClearResult();

    ui->RequestURL->setFocus(Qt::OtherFocusReason);
}


bool CRequestGUI::Store(QSettings& settings) const
{
	// Request
    settings.beginGroup("Request");
    settings.setValue("RequestURL", ui->RequestURL->text());
    settings.setValue("RequestType", ui->RequestType->currentText());
    settings.setValue("RequestBody", ui->RequestBody->toPlainText());
	settings.endGroup();

    // Request headers
    settings.beginGroup("RequestHeaders");
    ui->RequestHeaders->Store(settings);
    settings.endGroup();

	// Request parameters
    settings.beginGroup("RequestParams");
    ui->RequestParams->Store(settings);
	settings.endGroup();
    
	// Autentication settings
	settings.beginGroup("Authentication");
	settings.endGroup();

    // UI
    settings.beginGroup("UI");
	settings.setValue("SplitterState", ui->splitter->saveState());
	settings.endGroup();

    return true;
}


bool CRequestGUI::Restore(QSettings& settings)
{
    ui->RequestURL->blockSignals(true);
    ui->RequestType->blockSignals(true);
    ui->RequestHeaders->blockSignals(true);
    ui->RequestParams->blockSignals(true);

    // Request
    settings.beginGroup("Request");
	auto requestUrl = settings.value("RequestURL", "").toByteArray();
	QString requestUrlStr = QUrl::fromPercentEncoding(requestUrl);
	ui->RequestURL->setText(requestUrlStr);
	QString verb = settings.value("RequestType", "GET").toString();
	ui->RequestType->setCurrentText(verb);
	ui->RequestBody->setPlainText(settings.value("RequestBody", "").toString());
    settings.endGroup();

    // Request headers
    settings.beginGroup("RequestHeaders");
	ui->RequestHeaders->Restore(settings);
    settings.endGroup();

    // Request parameters
    settings.beginGroup("RequestParams");
    ui->RequestParams->Restore(settings);
    settings.endGroup();

    // Autentication settings
    settings.beginGroup("Authentication");
    settings.endGroup();

    // UI
    settings.beginGroup("UI");
	ui->splitter->restoreState(settings.value("SplitterState").toByteArray());
    settings.endGroup();

    // done
    ui->RequestURL->blockSignals(false);
    ui->RequestType->blockSignals(false);
    ui->RequestHeaders->blockSignals(false);
    ui->RequestParams->blockSignals(false);

    // request title
    QString requestTitle = verb + " " + requestUrlStr;
    Q_EMIT RequestTitleChanged(requestTitle);

    return true;
}


bool CRequestGUI::IsDefault() const
{
    // Implement logic to determine if the request is default.  
    // For example, check if certain fields are empty or have default values.  
    return ui->RequestURL->text().isEmpty() 
        && ui->RequestParams->rowCount() == 0 
        && ui->RequestBody->toPlainText().isEmpty();
}


void CRequestGUI::on_AddHeader_clicked()
{
	AddRequestHeader("<New Header>", "");
}


void CRequestGUI::on_RemoveHeader_clicked()
{
    auto currentRow = ui->RequestHeaders->currentRow();
    if (currentRow < 0) {
        return; // No row selected, do nothing
	}

    auto asked = QMessageBox::question(this, tr("Remove Header"),
        tr("Are you sure you want to remove the selected header?"), 
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (asked != QMessageBox::Yes) {
        return; // User chose not to remove the header
	}

    // Remove the currently selected row from the RequestHeaders table
    ui->RequestHeaders->DeleteCurrentRow();
}


void CRequestGUI::on_ClearHeaders_clicked()
{
    auto asked = QMessageBox::question(this, tr("Reset Headers"), 
        tr("Are you sure you want to reset all request headers to defaults?"), 
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (asked != QMessageBox::Yes) {
        return; // User chose not to clear headers
    }

	ui->RequestHeaders->setRowCount(0);
    SetDefaultHeaders();
	ui->RequestHeaders->setCurrentCell(0, 0); // Set focus to the first cell
}


void CRequestGUI::on_AddParameter_clicked()
{
    ui->RequestParams->blockSignals(true);
	ui->RequestParams->AddRowIfNotExists("<New Parameter>", "");
    ui->RequestParams->blockSignals(false);
}


void CRequestGUI::on_RemoveParameter_clicked()
{
    auto currentRow = ui->RequestParams->currentRow();
    if (currentRow < 0) {
        return; // No row selected, do nothing
    }

    auto asked = QMessageBox::question(this, tr("Remove Parameter"),
        tr("Are you sure you want to remove the selected parameter?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (asked != QMessageBox::Yes) {
        return;
    }

    // Remove the currently selected row
    ui->RequestParams->DeleteCurrentRow();

    // Update URL
    RebuildURL();
}


void CRequestGUI::on_ClearParameters_clicked()
{
    if (ui->RequestParams->rowCount() == 0)
        return;

    auto asked = QMessageBox::question(this, tr("Reset Parameters"),
        tr("Are you sure you want to remove all the parameters?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (asked != QMessageBox::Yes) {
        return; // User chose not to clear headers
    }

    ui->RequestParams->setRowCount(0);

    // Update URL
    RebuildURL();
}


void CRequestGUI::on_RequestURL_editingFinished()
{
    QString verb = ui->RequestType->currentText();
	QString request = ui->RequestURL->text().trimmed();
    
    // request title
    QString requestTitle = verb + " " + request;
    Q_EMIT RequestTitleChanged(requestTitle);
    
    // parse URL and update request parameters
    QUrl sourceUrl(request);
    //if (!sourceUrl.isValid()) {
    //    QMessageBox::warning(this, tr("Invalid URL"), tr("The provided URL is not valid."));
    //    return;

    // Clear existing active parameters
    ui->RequestParams->DeleteActiveRows();

    // Extract query parameters from the URL
    auto queryItems = sourceUrl.query();
    if (queryItems.isEmpty())
        return;

    ui->RequestParams->setUpdatesEnabled(false);
    ui->RequestParams->blockSignals(true);

	auto queryPairs = queryItems.split('&', Qt::SkipEmptyParts);
    for (const auto& pair : queryPairs) {
        auto keyValue = pair.split('=');
		QString key, value;
        key = QUrl::fromPercentEncoding(keyValue[0].toUtf8());
        if (keyValue.size() == 2) {
            value = QUrl::fromPercentEncoding(keyValue[1].toUtf8());
        }

		// if parameter and value already exist, aktivate it
		int foundRow = ui->RequestParams->FindRow(key, value);
        if (foundRow >= 0) {
            ui->RequestParams->SetActive(foundRow, true);
        } else {
            // Add new parameter if it doesn't exist
            ui->RequestParams->AddRow(key, value, true);
		}
    }

    ui->RequestParams->setUpdatesEnabled(true);
    ui->RequestParams->blockSignals(false);
}


void CRequestGUI::on_RequestParams_cellChanged(int /*row*/, int /*column*/)
{
    RebuildURL();
}


void CRequestGUI::on_RequestDataType_currentIndexChanged(int index)
{
    switch (index)
    {
    case DT_PLAIN:
        m_requestHL->setDocument(nullptr);
        AddRequestHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
	    break;

    case DT_JSON:
		m_requestHL->setDocument(ui->RequestBody->document());
        m_requestHL->setCurrentLanguage(QSourceHighlite::QSourceHighliter::CodeJSON);
        AddRequestHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        break;

    case DT_HTML:
        m_requestHL->setDocument(ui->RequestBody->document());
        m_requestHL->setCurrentLanguage(QSourceHighlite::QSourceHighliter::CodeXML);
        AddRequestHeader(QNetworkRequest::ContentTypeHeader, "text/html");
        break;

    default:
        m_requestHL->setDocument(nullptr);
        AddRequestHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
        break;
    }
}


void CRequestGUI::on_LoadRequestBody_clicked()
{
}


void CRequestGUI::SetDefaultHeaders()
{
    AddRequestHeader(QNetworkRequest::UserAgentHeader, qApp->applicationName() + " " + qApp->applicationVersion());
    AddRequestHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
}


void CRequestGUI::AddRequestHeader(const QString& name, const QString& value)
{
    ui->RequestHeaders->AddRowIfNotExists(name, value);
}


void CRequestGUI::AddRequestHeader(QNetworkRequest::KnownHeaders type, const QString& value)
{
    AddRequestHeader(CRequestManager::GetKnownHeader(type), value);
}


void CRequestGUI::RemoveRequestHeader(const QString& name)
{
    ui->RequestHeaders->DeleteActiveRows(name);
}


void CRequestGUI::RemoveRequestHeader(QNetworkRequest::KnownHeaders type)
{
    RemoveRequestHeader(CRequestManager::GetKnownHeader(type));
}


QNetworkCacheMetaData::RawHeaderList CRequestGUI::GetRequestHeaders() const
{
	auto activeHeaders = ui->RequestHeaders->GetEnabledParams();
    if (activeHeaders.isEmpty()) {
        return {}; // No headers to return
	}

    QNetworkCacheMetaData::RawHeaderList headers;

    for (const auto& header : activeHeaders) {
        headers.append({ header.first.toUtf8(), header.second.toUtf8() });
	}

    return headers;
}


void CRequestGUI::RebuildURL()
{
	ui->RequestURL->blockSignals(true); // Block signals to prevent infinite loop

    QUrl sourceUrl(ui->RequestURL->text());

    QString scheme = sourceUrl.scheme();
    QString user = sourceUrl.userInfo();
    QString password = sourceUrl.password();
    QString host = sourceUrl.host();
	QString query = sourceUrl.query();
	QString port = QString::number(sourceUrl.port(-1)); // -1 means use default port for the scheme
	QString fragment = sourceUrl.fragment();

    // cut parameters from the URL
    QUrl targetUrl(sourceUrl.adjusted(QUrl::RemoveQuery));
	
	//targetUrl.setQuery(""); // Clear existing query parameters
	auto params = ui->RequestParams->GetEnabledParams();
    if (!params.isEmpty()) {
        QStringList queryParts;
        for (const auto& param : params) {
            queryParts.append(QString("%1=%2").arg(param.first, param.second));
        }
        targetUrl.setQuery(queryParts.join("&")); // Set new query parameters
	}

    // update login:password
    //targetUrl.setUserInfo("");

	qDebug() << targetUrl.toString(QUrl::FullyEncoded);

	ui->RequestURL->setText(targetUrl.toString(QUrl::FullyEncoded));

	ui->RequestURL->blockSignals(false); // Unblock signals
}


void CRequestGUI::on_Run_clicked()
{
    LockRequest();
    ClearResult();

    QString request = ui->RequestURL->text().trimmed();
    QString verb = ui->RequestType->currentText();
    QString payload = ui->RequestBody->toPlainText();

    if (request.isEmpty()){
		UnlockRequest();
        ui->ResultCode->setText(tr("ERROR"));
        ui->ResponseText->appendPlainText(tr("Request is empty"));
        return;
    }

    auto reply = m_reqMgr.SendRequest(this, verb.toLocal8Bit(), QUrl(request), payload.toLocal8Bit(), GetRequestHeaders());

    m_timer.start();

    if (reply == nullptr) {
        UnlockRequest();
        ui->ResultCode->setText(tr("ERROR"));
        ui->ResponseText->appendPlainText(tr("Request could not be processed"));
        return;
    }

    ui->ResultCode->setText(tr("IN PROGRESS..."));
}


// process responses

void CRequestGUI::OnRequestDone()
{
    auto ms = m_timer.elapsed();
    auto reply = qobject_cast<QNetworkReply*>(sender());

    // update response time and size
    ui->ResponseSizeLabel->setText(tr("%1 bytes").arg(reply->bytesAvailable()));
    ui->TimeLabel->setText(tr("%1 ms").arg(ms));

    // update headers
    const auto& headers = reply->rawHeaderPairs();
    ui->ResponseHeaders->setRowCount(headers.size());

    int r = 0;
    for (const auto& header : headers) {
        auto& key = header.first;
        auto& value = header.second;
        ui->ResponseHeaders->setItem(r, 0, new QTableWidgetItem(key));
        ui->ResponseHeaders->setItem(r, 1, new QTableWidgetItem(value));
        r++;
    }

    // if error happened, this will be handled in another slot
    if (reply->error() != QNetworkReply::NoError)
        return;

    reply->deleteLater(); // delete reply object after processing

    UnlockRequest();

    ui->ReplyDataType->show();
    ui->ReplyDataInfo->show();

    // update status
    auto errorCode = reply->error();
    auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    auto statusReason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();

    auto statusText = tr("DONE: %1").arg(statusCode);
    if (statusCode == 200)
        statusText += " [OK]";
    else
        if (statusReason.isEmpty() && reply->errorString().isEmpty()) {
        }
        else {
            statusText += " [";
            if (statusReason.size())
                statusText += statusReason;
            if (reply->errorString().size())
                statusText += " " + reply->errorString();
            statusText += "]";
        }
    ui->ResultCode->setText(statusText);

    // update body
    m_replyData = reply->readAll();
    DecodeReply(reply, m_replyData);
}


void CRequestGUI::OnRequestError(QNetworkReply::NetworkError code)
{
    auto ms = m_timer.elapsed();
    ui->TimeLabel->setText(tr("%1 ms").arg(ms));

    UnlockRequest();

    ui->ReplyDataType->hide();
    ui->ReplyDataInfo->hide();

    auto reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater(); // delete reply object after processing

    // update response size
    ui->ResponseSizeLabel->setText(tr("%1 bytes").arg(reply->bytesAvailable()));

    // update status
    auto statusReason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();

    QString statusText = tr("FAILED: %1").arg((int)code);
    if (statusReason.size())
        statusText += tr(" [%1]").arg(statusReason);

    ui->ResultCode->setText(statusText);

    auto errorText = reply->errorString();
    if (!errorText.isEmpty()) {
        ShowPlainText(errorText, true);
    }

    auto result = reply->readAll();
    if (!result.isEmpty()) {
        ShowPlainText(result, true);
    }
}


void CRequestGUI::LockRequest()
{
    ui->Run->setEnabled(false);
    ui->RequestURL->setEnabled(false);
    ui->RequestType->setEnabled(false);
    ui->RequestBody->setEnabled(false);
    ui->RequestHeaders->setEnabled(false);
    ui->AddHeader->setEnabled(false);
    ui->RemoveHeader->setEnabled(false);
    ui->ClearHeaders->setEnabled(false);
    ui->ReplyDataType->setEnabled(false);
}


void CRequestGUI::UnlockRequest()
{
    ui->Run->setEnabled(true);
    ui->RequestURL->setEnabled(true);
    ui->RequestType->setEnabled(true);
    ui->RequestBody->setEnabled(true);
    ui->RequestHeaders->setEnabled(true);
    ui->AddHeader->setEnabled(true);
    ui->RemoveHeader->setEnabled(true);
    ui->ClearHeaders->setEnabled(true);
    ui->ReplyDataType->setEnabled(true);
}


void CRequestGUI::ClearResult()
{
    ui->ResponsePreview->clear();
    ui->ResponseText->clear();
    ui->ResponseHeaders->setRowCount(0);
    ui->ResponseHeaders->setColumnCount(2);
    ui->ResponseHeaders->setHorizontalHeaderLabels({ tr("Name"), tr("Value") });
    ui->TimeLabel->clear();
    ui->ResponseSizeLabel->clear();
    ui->ResultTabs->setCurrentIndex(0);
    ui->ReplyStack->setCurrentIndex(2);
    m_replyHL->setDocument(nullptr);
}


void CRequestGUI::DecodeReply(QNetworkReply* reply, const QByteArray& data)
{
    // Check the content type of the reply  
    auto contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();

    // images
    if (contentType.contains("image/", Qt::CaseInsensitive)) {
        bool ok = ShowReplyContent(DT_IMAGE, data, contentType);
    }
    // text, JSON, HTML...
    else if (contentType.contains("application/json", Qt::CaseInsensitive)) {
        ShowReplyContent(DT_JSON, data, contentType);
    }
    else if (contentType.contains("text/html", Qt::CaseInsensitive)) {
        ShowReplyContent(DT_HTML, data, contentType);
    }
    else if (contentType.contains("text/plain", Qt::CaseInsensitive)) {
        ShowReplyContent(DT_PLAIN, data, contentType);
    }
    else {
        // For other content types, display as binary data
        ShowReplyContent(DT_HEX, data, contentType);
    }
}


void CRequestGUI::ShowPlainText(const QString& text, bool append)
{
	ui->ReplyDataInfo->clear();

    if (append)
        ui->ResponseText->appendPlainText(text);
    else
        ui->ResponseText->setPlainText(text);
    
    ui->ReplyStack->setCurrentIndex(2);
}


bool CRequestGUI::ShowReplyContent(ReplyDisplayType showType, const QByteArray& data, const QString& contentType)
{
    int cbIndex = (int)showType;
    ui->ReplyDataType->setCurrentIndex(cbIndex);

    switch (showType)
    {
    case DT_IMAGE:
    {
        ui->ReplyStack->setCurrentIndex(0);

        QImage pm;
        if (pm.loadFromData(data))
        {
            QBuffer buffer(&m_replyData);
            buffer.open(QIODevice::ReadOnly);
            QImageReader reader(&buffer);
            QString format = reader.format().toUpper();

            ui->ResponsePreview->document()->addResource(QTextDocument::ImageResource, QUrl("image://pm"), pm);
            ui->ResponsePreview->setHtml(QString("<img src='image://pm'/>"));
			ui->ReplyDataInfo->setText(QString("   %3   %1x%2").arg(pm.width()).arg(pm.height()).arg(format));
        }
        else
        {
			ui->ReplyDataInfo->clear();
            ui->ResponsePreview->setHtml(tr("<font color=red>Cannot decode image format</font>"));
            return false;
        }
    }
    break;

    case DT_HEX:
    {
        if (!m_hexView) {
            m_hexView = new QHexView(ui->ReplyStack);
            ui->ReplyStack->addWidget(m_hexView);
        }

        ui->ReplyStack->setCurrentWidget(m_hexView);

        m_hexView->setData(new QHexView::DataStorageArray(m_replyData));
    }
        break;

    case DT_HTML:
        m_replyHL->setDocument(ui->ResponseText->document());
        m_replyHL->setCurrentLanguage(QSourceHighlite::QSourceHighliter::CodeXML);
        ShowPlainText(QString::fromUtf8(data), false);
        break;

    case DT_JSON:
        m_replyHL->setDocument(ui->ResponseText->document());
        m_replyHL->setCurrentLanguage(QSourceHighlite::QSourceHighliter::CodeJSON);
        ShowPlainText(QString::fromUtf8(data), false);
        break;

    default:    // plain text
        m_replyHL->setDocument(nullptr);
        ShowPlainText(QString::fromUtf8(data), false);
        break;
    }

    return true;
}


void CRequestGUI::on_ReplyDataType_currentIndexChanged(int index)
{
	ShowReplyContent((ReplyDisplayType)index, m_replyData);
}


