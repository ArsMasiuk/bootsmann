#include "CWorkspaceGUI.h"
#include "ui_CWorkspaceGUI.h"

#include "CRequestGUI.h"

#include <QTabBar>
#include <QToolButton>
#include <QFileDialog>


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
	addButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    addButton->setToolTip(tr("Create new HTTP request"));
    addButton->setIcon(QIcon(":/Icons/HTTP"));
    addButton->setShortcut(Qt::CTRL | Qt::Key_N);

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


bool CWorkspaceGUI::SaveCurrentRequest()
{
	auto currentTab = ui->Tabs->currentWidget();
	if (!currentTab)
        return false;

	auto requestUI = dynamic_cast<CRequestGUI*>(currentTab);
    if (!requestUI)
		return false;

	QString savePath = QFileDialog::getSaveFileName(this, tr("Save Request"), QString(), tr("Bootsmann Requests (*.bor);;All Files (*)"));
	if (savePath.isEmpty())
		return false;

	QSettings settings(savePath, QSettings::IniFormat);
	return requestUI->Store(settings);
}


bool CWorkspaceGUI::LoadRequest()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Load Request"), "", tr("Bootsmann Requests (*.bor);;All Files (*)"));
    if (filePath.isEmpty())
        return false;

    int index = AddRequestTab();
    auto requestUI = dynamic_cast<CRequestGUI*>(ui->Tabs->currentWidget());
    if (!requestUI)
        return false;

    QSettings settings(filePath, QSettings::IniFormat);
    return requestUI->Restore(settings);
}


int CWorkspaceGUI::AddRequestTab()
{
    auto requestUI = new CRequestGUI(m_reqMgr, this);
    int index = ui->Tabs->addTab(requestUI, tr("New Request"));
    ui->Tabs->setCurrentIndex(index);
    requestUI->Init();

    connect(requestUI, &CRequestGUI::RequestTitleChanged, this, [=](const QString& title){
		ui->Tabs->setTabToolTip(index, title);

        // truncate long titles
        if (title.size() > 20)
            ui->Tabs->setTabText(index, title.left(20) + "..."); 
        else
		    ui->Tabs->setTabText(index, title);
	});

    return index;
}


void CWorkspaceGUI::CloseRequestTab(int index)
{
    auto pageUI = ui->Tabs->widget(index);
    ui->Tabs->removeTab(index);
    pageUI->deleteLater();
}
