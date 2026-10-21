// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "DecryptionDialog.h"
#include "ui_DecryptionDialog.h"

#include "Application.h"
#include "CryptoDoc.h"
#include "IKValidator.h"
#include "QCryptoBackend.h"
#include "QPCSC.h"
#include "SslCertificate.h"
#include "dialogs/WaitDialog.h"
#include "dialogs/WarningDialog.h"
#include "effects/Overlay.h"
#include "widgets/CardListItem.h"

#include <QtCore/QPointer>
#include <QtCore/QTimer>

#include <memory>

DecryptionDialog::DecryptionDialog(CryptoDoc *doc, std::vector<CKey> keys, const QString &recipient, QWidget *parent)
	: QDialog(parent)
	, cryptoDoc(doc)
	, m_keys(std::move(keys))
	, ui(new Ui::DecryptionDialog)
{
	new Overlay(this);
	ui->setupUi(this);
	ui->progressPage->hide();
	setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint);
#ifdef Q_OS_WIN
	ui->buttonLayout->setDirection(QBoxLayout::RightToLeft);
#endif

	ui->recipientIcon->load(QStringLiteral(":/images/icon_Minu_eID_hover.svg"));
	ui->recipientName->setText(recipient);

	ui->tabs->addTab({});
	ui->tabs->addTab({});
	ui->tabs->addTab({});
	updateTabs();
	// Tab bar: a card's lock opens only with the card, a key share only with
	// Mobile-ID or Smart-ID, so the methods none of the recipient's locks open are hidden
	const bool share = shareKey();
	const bool card = std::ranges::any_of(m_keys, [](const CKey &key) { return key.lock.isPKI(); });
	ui->tabs->setTabVisible(IDCard, card);
	ui->tabs->setTabVisible(MobileID, share);
	ui->tabs->setTabVisible(SmartID, share);
	ui->tabs->setVisible(share);
	connect(ui->tabs, &QTabBar::currentChanged, ui->tabStack, &QStackedWidget::setCurrentIndex);
<<<<<<< HEAD
	connect(ui->tabs, &QTabBar::currentChanged, this, &DecryptionDialog::updateShareButton);
	ui->tabs->setCurrentIndex(share ? MobileID : IDCard);
=======
	connect(ui->tabs, &QTabBar::currentChanged, this, [this](int index) {
		if(index == IDCard)
			updateCards();
		else
			updateShareButton();
	});
	ui->tabs->setCurrentIndex(card ? IDCard : MobileID);
>>>>>>> f093edaf (Lock grouping)
	ui->tabStack->setCurrentIndex(ui->tabs->currentIndex());

	// ID-card tab
	ui->noCardIcon->load(QStringLiteral(":/images/icon_IDkaart_red.svg"));
	updateCards();
	connect(qApp->cryptoManager(), &QCryptoManager::cacheChanged, this, &DecryptionDialog::updateCards);
	ui->pin->setPinLen(4); // PIN1 (authentication) is 4..12 digits
	connect(ui->pin, &QLineEdit::textEdited, this, [this] {
		if(!ui->errorPin->text().isEmpty()) {
			ui->errorPin->clear();
			ui->errorPin->setVisible(false);
			ui->pin->setLabel({});
		}
		ui->decrypt->setEnabled(ui->pin->hasAcceptableInput());
	});
	connect(ui->pin, &QLineEdit::returnPressed, ui->decrypt, &QPushButton::click);

	// Mobile-ID and Smart-ID tabs: the lock names the recipient, shown in the header,
	// so only the Mobile-ID phone number is asked for
	ui->phoneNo->setValidator(new NumberValidator(ui->phoneNo));
	ui->phoneNo->setAttribute(Qt::WA_MacShowFocusRect, false);
	ui->errorPhone->hide();
	connect(ui->phoneNo, &QLineEdit::textEdited, this, &DecryptionDialog::updateShareButton);
	connect(ui->phoneNo, &QLineEdit::returnPressed, ui->decrypt, &QPushButton::click);

	connect(ui->decrypt, &QPushButton::clicked, this, [this] {
		if(currentMethod() == IDCard ? performDecrypt() : performShareDecrypt())
			accept();
	});
	connect(ui->progressCancel, &QPushButton::clicked, this, [this] {
		ui->progressCancel->setEnabled(false);
		cryptoDoc->cancel();
	});
	connect(ui->cancel, &QPushButton::clicked, this, &QDialog::reject);
	if(share)
		updateShareButton();
	connect(this, &QDialog::finished, this, &QDialog::close);
}

DecryptionDialog::~DecryptionDialog()
{
	delete ui;
}

DecryptionDialog::Method DecryptionDialog::currentMethod() const
{
	return static_cast<Method>(ui->tabs->currentIndex());
}

void DecryptionDialog::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
		updateTabs();
		updateNoCard();
		updateProgressInfo();
	}
	QDialog::changeEvent(event);
}

void DecryptionDialog::reject()
{
	if(ui->progressPage->isVisible())
	{
		if(ui->progressCancel->isEnabled())
			ui->progressCancel->click();
		return;
	}
	QDialog::reject();
}

void DecryptionDialog::updateTabs()
{
	ui->tabs->setTabText(IDCard, tr("ID-card"));
	ui->tabs->setTabText(MobileID, tr("Mobile-ID"));
	ui->tabs->setTabText(SmartID, tr("Smart-ID"));
}

void DecryptionDialog::updateProgressInfo()
{
	if(currentMethod() == MobileID)
		ui->progressInfo->setText(m_confirmDecrypt ?
			tr("Confirm decryption: make sure the control code matches the one on your phone screen and enter your Mobile-ID PIN1.") :
			tr("Log in: make sure the control code matches the one on your phone screen and enter your Mobile-ID PIN1."));
	else
		ui->progressInfo->setText(m_confirmDecrypt ?
			tr("Confirm decryption: make sure the control code matches the one in your Smart-ID app and enter your PIN1.") :
			tr("Log in: make sure the control code matches the one in your Smart-ID app and enter your PIN1."));
}

void DecryptionDialog::updateNoCard()
{
	if(!QPCSC::instance().serviceRunning())
		ui->noCardStatus->setText(tr("The PCSC service, required for using the ID-card, is not working. Check your computer settings."));
	else if(QPCSC::instance().readers().isEmpty())
		ui->noCardStatus->setText(tr("No readers found"));
	else
		ui->noCardStatus->setText(tr("No card in reader"));
	ui->noCard->setAccessibleDescription(ui->noCardStatus->text());
}

const CKey* DecryptionDialog::keyFor(const TokenData &token) const
{
	// A card can be addressed both directly and through the key server;
	// the direct lock needs no network and cannot expire on the server.
	const CKey *found = nullptr;
	for(const CKey &key: m_keys) {
		if(key.canDecrypt(token.cert()) && (!found || found->lock.type == libcdoc::Lock::SERVER))
			found = &key;
	}
	return found;
}

const CKey* DecryptionDialog::shareKey() const
{
	auto it = std::ranges::find(m_keys, libcdoc::Lock::SHARE_SERVER, [](const CKey &key) { return key.lock.type; });
	return it != m_keys.cend() ? &*it : nullptr;
}

void DecryptionDialog::updateCards()
{
	qDeleteAll(ui->cardListContainer->findChildren<CardListItem*>());

	// Only a card one of the locks is addressed to can open it; the others are listed
	// but not selectable, so it stays visible which card is missing or blocked.
	auto usableToken = [this](const TokenData &token) {
		SslCertificate c(token.cert());
		return c.isValid() && !token.data(QStringLiteral("blocked")).toBool() && keyFor(token);
	};

	const QList<TokenData> tokens = qApp->cryptoManager()->cache();
	if(std::none_of(tokens.cbegin(), tokens.cend(),
		[this](const TokenData &t) { return t == m_selectedToken; }))
	{
		m_selectedToken = {};
		for(const TokenData &token: tokens)
		{
			if(!SslCertificate(token.cert()).isEncryptionCert())
				continue;
			if(m_selectedToken.isNull())
				m_selectedToken = token;
			if(usableToken(token))
			{
				m_selectedToken = token;
				break;
			}
		}
	}

	bool any = false;
	CardListItem *selectedCard = nullptr;
	for(const TokenData &token: tokens) {
		if(!SslCertificate(token.cert()).isEncryptionCert())
			continue;
		auto *w = new CardListItem(ui->cardListContainer);
		w->update(token, token == m_selectedToken, keyFor(token) ?
			nullptr : QT_TRANSLATE_NOOP("CardListItem", "This card cannot decrypt the container"));
		connect(w, &CardListItem::selected, this, &DecryptionDialog::updateSelection);
		ui->cardListContainer->layout()->addWidget(w);
		w->show(); // In a dialog already shown it would stay hidden, and out of the layout, until later
		any = true;
		if(token == m_selectedToken)
			selectedCard = w;
	}

	if(!any)
		updateNoCard();
	ui->noCard->setVisible(!any);
	ui->cardList->setVisible(any);
	if(selectedCard)
		QMetaObject::invokeMethod(ui->cardList, [list = ui->cardList, card = QPointer<CardListItem>(selectedCard)] {
			if(card)
				list->ensureWidgetVisible(card, 0, 0);
		}, Qt::QueuedConnection);
	ui->labelCert->setVisible(any);
	updateSelection();
}

void DecryptionDialog::updateSelection()
{
	for(auto *w: ui->cardListContainer->findChildren<CardListItem*>()) {
		if(!w->isChecked())
			continue;
		if(!(w->token() == m_selectedToken)) {
			ui->pin->clear();
			ui->pin->setLabel({});
			ui->errorPin->clear();
		}
		if(currentMethod() == IDCard)
			w->refresh();
		m_selectedToken = w->token();
		break;
	}
	updatePinControls();
}

void DecryptionDialog::updatePinControls()
{
	bool usable = std::ranges::any_of(ui->cardListContainer->findChildren<CardListItem*>(),
		[](CardListItem *w) { return w->isChecked() && w->isEnabled(); });
	// The inline PIN1 field applies only to software PIN entry: PKCS11 readers
	// (non-Windows) that are not PinPads. PinPad and Windows keep their own PIN UI
	// (hardware / OS dialog) via the empty-pin path in performDecrypt().
	bool showPin = false;
#ifndef Q_OS_WIN
	showPin = usable && !m_selectedToken.data(QStringLiteral("pinpad")).toBool();
#endif
	ui->labelPin->setVisible(showPin);
	ui->pin->setVisible(showPin);
	ui->errorPin->setVisible(showPin && !ui->errorPin->text().isEmpty());
	ui->layoutPin->invalidate();
	if(!showPin)
		ui->pin->clear();

	if(currentMethod() == IDCard)
		ui->decrypt->setEnabled(usable && (!showPin || ui->pin->hasAcceptableInput()));
}

bool DecryptionDialog::performDecrypt()
{
	// Software PIN entry uses the inline field; PinPad/Windows pass an empty
	// PIN so login() falls back to the hardware/OS prompt.
	QString pin = ui->pin->isVisible() ? ui->pin->text() : QString();
	auto val = QCryptoBackend::getBackend(m_selectedToken, pin);
	if(!val) {
		switch(val.error()) {
		case QCryptoBackend::PinIncorrect:
			// Show the error under the field and let the user retype + retry.
			ui->pin->setLabel(QStringLiteral("error"));
			ui->errorPin->setText(QCryptoBackend::errorString(QCryptoBackend::PinIncorrect));
			ui->errorPin->setVisible(true);
			ui->pin->clear();
			ui->pin->setFocus();
			ui->decrypt->setEnabled(false);
			return false;
		case QCryptoBackend::PinCanceled:
			return false;
		default:
			WarningDialog::create(this)
				->withTitle(tr("Failed to decrypt document"))
				->withText(QCryptoBackend::errorString(val.error()))
				->exec();
			return false;
		}
	}
	std::unique_ptr<QCryptoBackend> backend(val.value());
	WaitDialogHolder waitDialog(this, tr("Decrypting"));
	const CKey *key = keyFor(m_selectedToken);
	if(!key)
		return false;
	return cryptoDoc->decrypt(key->lock, {}, std::move(backend));
}

void DecryptionDialog::updateShareButton()
{
	static const QStringList countryCodes {QStringLiteral("372"), QStringLiteral("370")};
	const QString phone = ui->phoneNo->text();
	ui->decrypt->setEnabled(currentMethod() == SmartID ||
		(phone.size() >= 8 && countryCodes.contains(phone.left(3))));
}

bool DecryptionDialog::performShareDecrypt()
{
	auto showPage = [this](QWidget *page) {
		ui->formPage->setVisible(page == ui->formPage);
		ui->progressPage->setVisible(page == ui->progressPage);
	};
	ui->progressCode->clear();
	m_confirmDecrypt = false;
	updateProgressInfo();
	ui->progressBar->setValue(0);
	ui->progressCancel->setEnabled(true);
	showPage(ui->progressPage);

	// Each confirmation on the device gets its own code and its own time to answer
	QPointer<QTimer> timer = new QTimer(this);
	timer->setInterval(1000);
	connect(timer, &QTimer::timeout, this, [this] {
		ui->progressBar->setValue(std::min(ui->progressBar->value() + 1, ui->progressBar->maximum()));
	});
	QPointer<DecryptionDialog> self(this);
	auto feedback = [self, timer](int code) {
		if(!self || !timer)
			return;
		if(!self->ui->progressCode->text().isEmpty()) {
			self->m_confirmDecrypt = true;
			self->updateProgressInfo();
		}
		self->ui->progressCode->setText(QStringLiteral("%1").arg(code, 4, 10, QLatin1Char('0')));
		self->ui->progressBar->setValue(0);
		timer->start();
	};
	bool result = cryptoDoc->decrypt(shareKey()->lock,
		currentMethod() == MobileID ? ui->phoneNo->text() : QString(), feedback);
	if(!self)
		return result;
	timer->deleteLater();
	if(!result)
		showPage(ui->formPage);
	return result;
}
