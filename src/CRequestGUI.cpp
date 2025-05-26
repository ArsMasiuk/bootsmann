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
