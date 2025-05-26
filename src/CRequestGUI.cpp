#include "CRequestGUI.h"
#include "ui_CRequestGUI.h"

#include "CRequestManager.h"


CRequestGUI::CRequestGUI(CRequestManager& reqMgr, QWidget *parent)
    : QWidget(parent)
    , m_reqMgr(reqMgr)
    , ui(new Ui::CRequestGUI)
{
    ui->setupUi(this);

    connect(&m_reqMgr, &CRequestManager::RequestSuccess,
		this, &CRequestGUI::onRequestFinished);
	connect(&m_reqMgr, &CRequestManager::RequestError,
        this, &CRequestGUI::onRequestError);
}

CRequestGUI::~CRequestGUI()
{
    delete ui;
}


void CRequestGUI::Init()
{
    ui->RequestURL->setFocus(Qt::OtherFocusReason);
}


void CRequestGUI::onRequestFinished(int reqId, int code, const QByteArray& result)
{
    ui->OutputText->clear();

    ui->OutputText->appendPlainText(tr("Code: %1").arg(code));

    if (!result.isEmpty()) {
        ui->OutputText->appendPlainText(result);
    }
}


void CRequestGUI::onRequestError(int reqId, QNetworkReply::NetworkError code, const QString& errorMsg)
{
    ui->OutputText->clear();

    ui->OutputText->appendPlainText(tr("Error: %1").arg((int) code));

    if (!errorMsg.isEmpty()) {
        ui->OutputText->appendPlainText(errorMsg);
    }
}


void CRequestGUI::on_Run_clicked()
{
    ui->OutputText->clear();

    QString request = ui->RequestURL->text();
    if (request.isEmpty()){
        ui->OutputText->appendPlainText(tr("Request is empty"));
        return;
    }

    QString verb = ui->RequestType->currentText();

    QString payload = ui->InputText->toPlainText();

    m_reqMgr.SendRequest(verb.toLocal8Bit(), QUrl(request), payload.toLocal8Bit());
}
