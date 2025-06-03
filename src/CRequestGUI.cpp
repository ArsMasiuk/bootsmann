#include "CRequestGUI.h"
#include "ui_CRequestGUI.h"

#include "CRequestManager.h"

#include <QMessageBox>


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
}


CRequestGUI::~CRequestGUI()
{
    delete ui;
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
    // Request
    settings.beginGroup("Request");
	ui->RequestURL->setText(settings.value("RequestURL", "").toString());
	ui->RequestType->setCurrentText(settings.value("RequestType", "GET").toString());
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


    // request title
    QString requestTitle = ui->RequestType->currentText() + " " + ui->RequestURL->text();
    Q_EMIT RequestTitleChanged(requestTitle);

    return true;
}


void CRequestGUI::OnRequestSuccess()
{
    auto ms = m_timer.elapsed();
	ui->TimeLabel->setText(tr("%1 ms").arg(ms));

    UnlockRequest();

    auto reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater(); // delete reply object after processing

	// update response size
	ui->ResponseSizeLabel->setText(tr("%1 bytes").arg(reply->bytesAvailable()));

    // update status
    auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    auto statusReason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();

    auto statusText = tr("DONE. Code: %1").arg(statusCode);
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
	auto result = reply->readAll();
    if (!result.isEmpty()) {
        ui->ResponseBody->appendPlainText(result);
    }

	// update headers
    const auto &headers = reply->rawHeaderPairs();
	ui->ResponseHeaders->setRowCount(headers.size());

    int r = 0;
    for (const auto& header : headers) {
		auto &key = header.first;
        auto &value = header.second;
		ui->ResponseHeaders->setItem(r, 0, new QTableWidgetItem(key));
        ui->ResponseHeaders->setItem(r, 1, new QTableWidgetItem(value));
        r++;
    }
}


void CRequestGUI::OnRequestError(QNetworkReply::NetworkError code)
{
    auto ms = m_timer.elapsed();
    ui->TimeLabel->setText(tr("%1 ms").arg(ms));

	UnlockRequest();

    auto reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater(); // delete reply object after processing

    // update response size
    ui->ResponseSizeLabel->setText(tr("%1 bytes").arg(reply->bytesAvailable()));

    // update status
    auto statusReason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();

	QString statusText = tr("FAILED. Code: %1").arg((int)code);
    if (statusReason.size())
		statusText += tr(" [%1]").arg(statusReason);

	ui->ResultCode->setText(statusText);

    auto errorText = reply->errorString();
    if (!errorText.isEmpty()) {
        ui->ResponseBody->appendPlainText(errorText);
    }

    auto result = reply->readAll();
    if (!result.isEmpty()) {
        ui->ResponseBody->appendPlainText(result);
    }
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
}


void CRequestGUI::ClearResult()
{
    ui->ResponseBody->clear();
	ui->ResponseHeaders->setRowCount(0);
	ui->ResponseHeaders->setColumnCount(2);
    ui->ResponseHeaders->setHorizontalHeaderLabels({ tr("Name"), tr("Value") });
    ui->TimeLabel->clear();
	ui->ResponseSizeLabel->clear();
	ui->ResultTabs->setCurrentIndex(0);
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
        ui->ResponseBody->appendPlainText(tr("Request is empty"));
        return;
    }

    auto reply = m_reqMgr.SendRequest(this, verb.toLocal8Bit(), QUrl(request), payload.toLocal8Bit(), GetRequestHeaders());

    m_timer.start();

    if (reply == nullptr) {
        UnlockRequest();
        ui->ResultCode->setText(tr("ERROR"));
        ui->ResponseBody->appendPlainText(tr("Request could not be processed"));
        return;
    }

    ui->ResultCode->setText(tr("IN PROGRESS..."));
}
