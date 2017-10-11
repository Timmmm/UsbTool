#pragma once

#include <QSpinBox>

class PaddedSpinBox : public QSpinBox
{
public:
	PaddedSpinBox(QWidget *parent = 0) : QSpinBox(parent)
	{
	}
protected:
	QString textFromValue(int value) const override
	{
		// Pad to the width of maximum().
		int width = QString::number(maximum(), displayIntegerBase()).size();
		return QString("%1").arg(value, width, displayIntegerBase(), QChar('0')).toUpper();
	}
};
