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
    auto ms = m_timer.elapsed();
	ui->TimeLabel->setText(tr("%1 ms").arg(ms));

    auto reply = qobject_cast<QNetworkReply*>(sender());

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

	auto result = reply->readAll();

    ui->OutputText->clear();

    if (!result.isEmpty()) {
        ui->OutputText->appendPlainText(result);
    }

    reply->deleteLater(); // delete reply object after processing
}


void CRequestGUI::OnRequestError(QNetworkReply::NetworkError code)
{
    auto ms = m_timer.elapsed();
    ui->TimeLabel->setText(tr("%1 ms").arg(ms));

    auto reply = qobject_cast<QNetworkReply*>(sender());

    auto statusReason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();

	QString statusText = tr("FAILED. Code: %1").arg((int)code);
    if (statusReason.size())
		statusText += tr(" [%1]").arg(statusReason);

	ui->ResultCode->setText(statusText);

    ui->OutputText->clear();

    auto errorText = reply->errorString();
    if (!errorText.isEmpty()) {
        ui->OutputText->appendPlainText(errorText);
    }

    auto result = reply->readAll();
    if (!result.isEmpty()) {
        ui->OutputText->appendPlainText(result);
    }

    reply->deleteLater(); // delete reply object after processing
}


void CRequestGUI::on_Run_clicked()
{
    ui->OutputText->clear();
    ui->TimeLabel->clear();

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

    m_timer.start();

    if (reply == nullptr) {
        ui->ResultCode->setText(tr("ERROR"));
        ui->OutputText->appendPlainText(tr("Request could not be processed"));
        return;
    }

    ui->ResultCode->setText(tr("IN PROGRESS..."));
}
