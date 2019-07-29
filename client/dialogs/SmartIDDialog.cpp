#include "SmartIDDialog.h"
#include "ui_SmartIDDialog.h"

#include "Settings.h"
#include "Styles.h"
#include "effects/Overlay.h"

SmartIDDialog::SmartIDDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::SmartIDDialog)
{
	Overlay *overlay = new Overlay(parent->topLevelWidget());
	overlay->show();
	connect(this, &SmartIDDialog::destroyed, overlay, &Overlay::deleteLater);

	ui->setupUi(this);
	setWindowFlags( Qt::Dialog | Qt::FramelessWindowHint );
	setWindowModality( Qt::ApplicationModal );
	setFixedSize( size() );
	QFont condensed12 = Styles::font(Styles::Condensed, 12);
	QFont condensed14 = Styles::font(Styles::Condensed, 14);
	ui->labelPhone->setFont(condensed12);
	ui->idCode->setFont(condensed12);
	ui->sign->setFont(condensed14);
	ui->cancel->setFont(condensed14);
	ui->idCode->setText(Settings(qApp->applicationName()).value(QStringLiteral("SmartID")).toString());
	connect(ui->sign, &QPushButton::clicked, this, &QDialog::accept);
	connect(ui->cancel, &QPushButton::clicked, this, &QDialog::reject);
	connect(ui->idCode, &QLineEdit::textEdited, this, [](const QString &text){
		Settings(qApp->applicationName()).setValueEx(QStringLiteral("SmartID"), text, QString());
	});
}

SmartIDDialog::~SmartIDDialog()
{
	delete ui;
}

QString SmartIDDialog::country() const
{
	return QStringLiteral("EE");
}

QString SmartIDDialog::idCode() const
{
	return ui->idCode->text();
}
