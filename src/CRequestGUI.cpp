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

    ui->RequestHeaders->setRowCount(0);
    ui->RequestHeaders->setColumnCount(2);
    ui->RequestHeaders->setHorizontalHeaderLabels({ tr("Name"), tr("Value") });
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
	auto asked = QMessageBox::question(this, tr("Remove Header"), 
        tr("Are you sure you want to remove the selected header?"), 
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (asked != QMessageBox::Yes) {
        return; // User chose not to remove the header
	}

    // Remove the currently selected row from the RequestHeaders table
    auto currentRow = ui->RequestHeaders->currentRow();
    if (currentRow >= 0) {
        ui->RequestHeaders->removeRow(currentRow);
        if (currentRow < ui->RequestHeaders->rowCount()) {
            ui->RequestHeaders->setCurrentCell(currentRow, 0);
        }
    } else {
        // No row selected, do nothing or show a message
	}
}


void CRequestGUI::on_ClearHeaders_clicked()
{
    auto asked = QMessageBox::question(this, tr("Clear Headers"), 
        tr("Are you sure you want to clear all request headers?"), 
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (asked != QMessageBox::Yes) {
        return; // User chose not to clear headers
    }

	//while (ui->RequestHeaders->rowCount() > 0)
 //       ui->RequestHeaders->removeRow(0);
	ui->RequestHeaders->setRowCount(0);
    SetDefaultHeaders();
	ui->RequestHeaders->setCurrentCell(0, 0); // Set focus to the first cell
}


void CRequestGUI::SetDefaultHeaders()
{
    AddRequestHeader(QNetworkRequest::UserAgentHeader, qApp->applicationDisplayName());
    AddRequestHeader(QNetworkRequest::ContentTypeHeader, "application/json");
}


void CRequestGUI::AddRequestHeader(const QString& name, const QString& value)
{
	// check if header already exists
    for (int i = 0; i < ui->RequestHeaders->rowCount(); ++i) {
        if (ui->RequestHeaders->item(i, 0)->text() == name) {
            ui->RequestHeaders->item(i, 1)->setText(value);
			ui->RequestHeaders->setCurrentCell(i, 0);
            return;
        }
    }

    // add new header
    int row = ui->RequestHeaders->rowCount();
    ui->RequestHeaders->insertRow(row);
    ui->RequestHeaders->setItem(row, 0, new QTableWidgetItem(name));
	ui->RequestHeaders->setItem(row, 1, new QTableWidgetItem(value));
    ui->RequestHeaders->setCurrentCell(row, 0);
}


void CRequestGUI::AddRequestHeader(QNetworkRequest::KnownHeaders type, const QString& value)
{
	QNetworkRequest tempRequest;
    tempRequest.setHeader(type, value);
    QString headerName = tempRequest.rawHeaderList().first();
    AddRequestHeader(headerName, value);
}


QNetworkCacheMetaData::RawHeaderList CRequestGUI::GetRequestHeaders() const
{
    QNetworkCacheMetaData::RawHeaderList headers;

    for (int i = 0; i < ui->RequestHeaders->rowCount(); ++i) {
        auto nameItem = ui->RequestHeaders->item(i, 0);
        auto valueItem = ui->RequestHeaders->item(i, 1);
        if (nameItem && valueItem) {
            QString name = nameItem->text().trimmed();
            QString value = valueItem->text().trimmed();
            if (!name.isEmpty()) {
                headers.append({ name.toUtf8(), value.toUtf8() });
            }
        }
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
