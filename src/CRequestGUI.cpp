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


void CRequestGUI::Store(QSettings& settings) const
{
	// Request
    settings.beginGroup("Request");
    settings.setValue("RequestURL", ui->RequestURL->text());
    settings.setValue("RequestType", ui->RequestType->currentText());
    settings.setValue("RequestBody", ui->RequestBody->toPlainText());
	settings.endGroup();

    // Request headers
    settings.beginGroup("RequestHeaders");
    //QNetworkCacheMetaData::RawHeaderList headers = GetRequestHeaders();
    //settings.setValue("RequestHeaders", QVariant::fromValue(headers));
    settings.endGroup();

	// Request parameters
    settings.beginGroup("RequestParams");
    //QNetworkCacheMetaData::RawHeaderList params = GetRequestParams();
    //settings.setValue("RequestParams", QVariant::fromValue(params));
	settings.endGroup();
    
	// Autentication settings
	settings.beginGroup("Authentication");
	settings.endGroup();

    // UI
    settings.beginGroup("UI");
	settings.setValue("SplitterState", ui->splitter->saveState());
	settings.endGroup();
}


void CRequestGUI::Restore(const QSettings& settings)
{
    // to do
}


void CRequestGUI::OnRequestSuccess()
{
    auto ms = m_timer.elapsed();
	ui->TimeLabel->setText(tr("%1 ms").arg(ms));

    UnlockRequest();

    auto reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater(); // delete reply object after processing

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
	ui->RequestParams->AddRowIfNotExists("<New Parameter>", "");
}


void CRequestGUI::on_RemoveParameter_clicked()
{
}


void CRequestGUI::on_ClearParameters_clicked()
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
	ui->ResultTabs->setCurrentIndex(0);
}


void CRequestGUI::on_Run_clicked()
{
    LockRequest();
    ClearResult();

    QString request = ui->RequestURL->text();
    QString verb = ui->RequestType->currentText();
    QString payload = ui->RequestBody->toPlainText();

    // request title
	QString requestTitle = verb + " " + request;
	Q_EMIT RequestTitleChanged(requestTitle);


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
