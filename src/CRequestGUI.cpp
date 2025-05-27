#include "CRequestGUI.h"
#include "ui_CRequestGUI.h"

#include "CRequestManager.h"


CRequestGUI::CRequestGUI(CRequestManager& reqMgr, QWidget *parent)
    : QWidget(parent)
    , m_reqMgr(reqMgr)
    , ui(new Ui::CRequestGUI)
{
    ui->setupUi(this);
}


CRequestGUI::~CRequestGUI()
{
    delete ui;
}


void CRequestGUI::Init()
{
    ui->RequestURL->setFocus(Qt::OtherFocusReason);
}


void CRequestGUI::OnRequestSuccess()
{
    auto reply = qobject_cast<QNetworkReply*>(sender());

    auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    auto statusText = statusCode == 200 ? "OK" : reply->errorString();

	auto result = reply->readAll();

    ui->ResultCode->setText(
        tr("DONE. Code: %1 [%2]").arg(statusCode).arg(statusText));

    ui->OutputText->clear();

    if (!result.isEmpty()) {
        ui->OutputText->appendPlainText(result);
    }

    reply->deleteLater(); // delete reply object after processing
}


void CRequestGUI::OnRequestError(QNetworkReply::NetworkError code)
{
    auto reply = qobject_cast<QNetworkReply*>(sender());

    auto statusText = reply->errorString();

    ui->ResultCode->setText(
        tr("FAILED. Code: %1").arg((int)code));

    ui->OutputText->clear();

    if (!statusText.isEmpty()) {
        ui->OutputText->appendPlainText(statusText);
    }

    reply->deleteLater(); // delete reply object after processing
}


void CRequestGUI::on_Run_clicked()
{
    ui->OutputText->clear();

    QString request = ui->RequestURL->text();
    QString verb = ui->RequestType->currentText();
    QString payload = ui->InputText->toPlainText();

    // request title
	QString requestTitle = verb + " " + request;
	Q_EMIT RequestTitleChanged(requestTitle);


    if (request.isEmpty()){
        ui->ResultCode->setText(tr("ERROR"));
        ui->OutputText->appendPlainText(tr("Request is empty"));
        return;
    }

    auto reply = m_reqMgr.SendRequest(this, verb.toLocal8Bit(), QUrl(request), payload.toLocal8Bit());

    if (reply == nullptr) {
        ui->ResultCode->setText(tr("ERROR"));
        ui->OutputText->appendPlainText(tr("Request could not be processed"));
        return;
    }

    ui->ResultCode->setText(tr("IN PROGRESS..."));
}
