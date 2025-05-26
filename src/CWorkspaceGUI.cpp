#include "CWorkspaceGUI.h"
#include "ui_CWorkspaceGUI.h"

#include "CRequestGUI.h"

#include <QTabBar>
#include <QToolButton>


const int INFO_TAB_INDEX = 0;
const int LOG_TAB_INDEX = 1;

CWorkspaceGUI::CWorkspaceGUI(CRequestManager& reqMgr, QWidget *parent)
:   m_reqMgr(reqMgr),
    QWidget(parent),
    ui(new Ui::CWorkspaceGUI)
{
    ui->setupUi(this);

    // INFO and LOG non-closable
    ui->Tabs->tabBar()->tabButton(INFO_TAB_INDEX, QTabBar::RightSide)->hide();
    ui->Tabs->tabBar()->tabButton(LOG_TAB_INDEX, QTabBar::RightSide)->hide();

    // temp: hide them
    ui->Tabs->tabBar()->setTabVisible(INFO_TAB_INDEX, false);
    ui->Tabs->tabBar()->setTabVisible(LOG_TAB_INDEX, false);

    // add tool button
    auto addButton = new QToolButton(this);
    addButton->setText("*");
    addButton->setToolTip(tr("Create new request"));
    addButton->setShortcut(Qt::CTRL + Qt::Key_N);

    connect(addButton, &QToolButton::clicked, this, [=](){
        AddRequestTab();
    });

    ui->Tabs->setCornerWidget(addButton, Qt::TopRightCorner);

    // handle closing tabs
    connect(ui->Tabs->tabBar(), &QTabBar::tabCloseRequested, this, &CWorkspaceGUI::CloseRequestTab);

    // add default request
    AddRequestTab();
}

CWorkspaceGUI::~CWorkspaceGUI()
{
    delete ui;
}


int CWorkspaceGUI::AddRequestTab()
{
    auto requestUI = new CRequestGUI(m_reqMgr, this);
    int index = ui->Tabs->addTab(requestUI, tr("New Request"));
    ui->Tabs->setCurrentIndex(index);
    requestUI->Init();
    return index;
}


void CWorkspaceGUI::CloseRequestTab(int index)
{
    auto pageUI = ui->Tabs->widget(index);
    ui->Tabs->removeTab(index);
    pageUI->deleteLater();
}
