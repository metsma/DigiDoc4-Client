// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "TokenData.h"

#include <QtWidgets/QAbstractButton>

namespace Ui { class CardListItem; }

class CardListItem final: public QAbstractButton
{
	Q_OBJECT

public:
	explicit CardListItem(QWidget *parent = nullptr);
	~CardListItem() final;

	TokenData token() const;
	void refresh();
	// reason: QT_TRANSLATE_NOOP("CardListItem", ...) why the card cannot be used, shown in
	// place of its validity; translated here, so it follows language changes. A card is
	// usable when its certificate is valid, its PIN not blocked and there is no reason.
	void update(const TokenData &token, bool selected, const char *reason = nullptr);

signals:
	void selected(const TokenData &token);

protected:
	void paintEvent(QPaintEvent *ev) final;
	void changeEvent(QEvent *ev) final;

private:
	Ui::CardListItem *ui;
	TokenData t;
	const char *reason = nullptr;
};
