/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#pragma once

#include <QString>
#include <QList>
#include <QUrl>
#include <QDateTime>
#include <QVariantMap>

namespace LeechCraft
{
namespace Azoth
{
namespace Murm
{
	struct ListInfo
	{
		qulonglong ID_;
		QString Name_;
	};

	struct UserInfo
	{
		qulonglong ID_;

		QString FirstName_;
		QString LastName_;
		QString Nick_;

		QUrl Photo_;
		QUrl BigPhoto_;

		int Gender_;
	
		QDate Birthday_;

		QString HomePhone_;
		QString MobilePhone_;

		int Timezone_;

		int Country_;
		int City_;

		bool IsOnline_;

		QList<qulonglong> Lists_;
	};

	enum MessageFlag
	{
		None =			0,
		Unread =		1 << 0,
		Outbox = 		1 << 1,
		Replied = 		1 << 2,
		Important = 	1 << 3,
		Chat = 			1 << 4,
		Friends = 		1 << 5,
		Spam = 			1 << 6,
		Deleted = 		1 << 7,
		Fixed = 		1 << 8,
		Media = 		1 << 9
	};

	Q_DECLARE_FLAGS (MessageFlags, MessageFlag)

	struct MessageInfo
	{
		qulonglong ID_;
		qulonglong From_;

		QString Text_;

		MessageFlags Flags_;

		QDateTime TS_;

		QVariantMap Params_;
	};

	enum class GeoIdType
	{
		Country,
		City
	};
}
}
}

Q_DECLARE_OPERATORS_FOR_FLAGS (LeechCraft::Azoth::Murm::MessageFlags)
