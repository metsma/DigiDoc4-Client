// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once
#include <QDialog>

#include "CryptoDoc.h"
#include "TokenData.h"

namespace Ui { class DecryptionDialog; }

class DecryptionDialog final : public QDialog
{
	Q_OBJECT
public:
	enum Method { IDCard, MobileID, SmartID };

	explicit DecryptionDialog(CryptoDoc *doc, const CKey &key, const QString &recipient,
		QWidget *parent = nullptr);
	~DecryptionDialog() final;

private:
	void changeEvent(QEvent *event) final;
	void reject() final;
	void updateCards();
	void updateSelection();
	void updatePinControls();
	void updateNoCard();
	void updateProgressInfo();
	void updateTabs();
	void updateShareButton();
	bool performDecrypt();
	bool performShareDecrypt();
	Method currentMethod() const;

	CryptoDoc *cryptoDoc;
	CKey m_key;
	Ui::DecryptionDialog *ui;
	TokenData m_selectedToken;
	bool m_confirmDecrypt = false;
};
