#pragma once

#include <QTableWidget>
#include <QList>
#include <QPair>

class CParamTable : public QTableWidget
{
	//Q_OBJECT

public:
	CParamTable(QWidget* parent = nullptr);

	int AddRow(const QString& name = "", const QString& value = "", bool on = true);
	int AddRowIfNotExists(const QString& name = "", const QString& value = "", bool on = true);
	bool DeleteRow(int row);
	bool DeleteCurrentRow();

	typedef QList<QPair<QString, QString>> ParamList;
	ParamList GetEnabledParams() const;
};

