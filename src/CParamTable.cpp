#include "CParamTable.h"

#include <QHeaderView>


CParamTable::CParamTable(QWidget* parent)
	: QTableWidget(parent)
{
	setColumnCount(3);
	setHorizontalHeaderLabels({ tr("Name"), tr("Value"), tr("Use") });
	horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
	setColumnWidth(2, 40);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);
}


int CParamTable::AddRow(const QString& name, const QString& value, bool on)
{
	int row = rowCount();
	insertRow(row);

	setItem(row, 0, new QTableWidgetItem(name));
	setItem(row, 1, new QTableWidgetItem(value));

	auto checkItem = new QTableWidgetItem(1);
	checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
	checkItem->setCheckState(on ? Qt::Checked : Qt::Unchecked);
	setItem(row, 2, checkItem);

	setCurrentCell(row, 0);

	return row;
}


int CParamTable::AddRowIfNotExists(const QString& name, const QString& value, bool on)
{
	// Check if the parameter already exists (by name)
	for (int i = 0; i < rowCount(); ++i) {
		QTableWidgetItem* item = this->item(i, 0);
		if (item && item->text() == name) {
			// Already exists, update value and return row index
			QTableWidgetItem* valueItem = this->item(i, 1);
			if (valueItem) {
				valueItem->setText(value);
			}
			else {
				setItem(i, 1, new QTableWidgetItem(value));
			}
			setCurrentCell(i, 0);
			return i;
		}
	}

	// Add new row
	return AddRow(name, value, on);
}


bool CParamTable::DeleteRow(int row)
{
	if (row < 0 || row >= rowCount())
		return false;

	removeRow(row);
	return true;
}


bool CParamTable::DeleteCurrentRow()
{
	int row = currentRow();
	
	if (row >= 0) {
		removeRow(row);
		if (row < rowCount()) {
			setCurrentCell(row, 0);
		}
		return true;
	}

	return false;
}


CParamTable::ParamList CParamTable::GetEnabledParams() const
{
	ParamList result;

	for (int row = 0; row < rowCount(); ++row) {
		auto item = this->item(row, 2);
		if (item && item->checkState() == Qt::Checked) {
			QString name = this->item(row, 0)->text();
			QString value = this->item(row, 1)->text();
			result.append({ name, value });
		}
	}

	return result;
}


// IO

void CParamTable::Store(QSettings& settings) const
{
	for (int row = 0; row < rowCount(); ++row) {
		QString name = item(row, 0)->text();
		QString value = item(row, 1)->text();
		bool on = (item(row, 2)->checkState() == Qt::Checked);
		//settings.setValue(name, QVariant::fromValue(std::make_pair(value, on)));
		auto key = QVariant::fromValue(std::make_pair(name, value)).toString();
		settings.setValue(key, on);
	}
}