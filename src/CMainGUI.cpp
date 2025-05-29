#include "CMainGUI.h"
#include "ui_CMainGUI.h"

#include "CWorkspaceGUI.h"
#include "CRequestManager.h"

#include <QVBoxLayout>


CMainGUI::CMainGUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CMainGUI)
{
    ui->setupUi(this);

    // style
    setStyleSheet(
        "QWidget {"
            "font-family: Verdana;"
            "font-size: 10pt;"
            "color: #333;"
        "}\n"

        "QWidget:!enabled {"
            "color: #888;"
        "}\n"

        "QToolButton {"
            "min-height: 28px;"
            "min-width: 28px;"
        "}\n"

        "QTabBar::tab {"
            "min-width: 100px;"
		    "min-height: 30px;"
        "}\n"
    );

    // init
	qApp->setApplicationDisplayName(qApp->applicationName() + " " + qApp->applicationVersion());
    setWindowTitle(qApp->applicationDisplayName());

    // add defautl request manager
    auto reqMgr = new CRequestManager(this);

    // add default workspace
    m_activeWorkspace = new CWorkspaceGUI(*reqMgr, this);
	setCentralWidget(m_activeWorkspace);
}


CMainGUI::~CMainGUI()
{
    delete ui;
}


void CMainGUI::on_actionExit_triggered()
{
	qApp->quit();
}


void CMainGUI::on_actionAbout_triggered()
{
	QApplication::aboutQt();
}


void CMainGUI::on_actionNewRequest_triggered()
{
    m_activeWorkspace->CreateNewRequest();
}
