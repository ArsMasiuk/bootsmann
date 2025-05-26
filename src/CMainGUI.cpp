#include "CMainGUI.h"
#include "ui_CMainGUI.h"

#include "CWorkspaceGUI.h"
#include "CRequestManager.h"

#include <QVBoxLayout>


CMainGUI::CMainGUI(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CMainGUI)
{
    ui->setupUi(this);

    // style
    setStyleSheet(
        "QWidget {"
            "font-family: Barlow;"
            "font-size: 10pt;"
        "}");

    // init
    setWindowTitle("The Bootsmann 1.0");

    // add defautl request manager
    auto reqMgr = new CRequestManager(this);

    // add default workspace
    setLayout(new QVBoxLayout);

    auto workspace = new CWorkspaceGUI(*reqMgr, this);
    layout()->addWidget(workspace);
}


CMainGUI::~CMainGUI()
{
    delete ui;
}
