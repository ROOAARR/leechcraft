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

#include "typedmatchers.h"
#include <QStringList>
#include <QWidget>
#include <QtDebug>
#include "ui_intmatcherconfigwidget.h"
#include "ui_stringlikematcherconfigwidget.h"

//Q_DECLARE_OPERATORS_FOR_FLAGS (LeechCraft::AdvancedNotifications::IntMatcher::Operations);

namespace LeechCraft
{
namespace AdvancedNotifications
{
	TypedMatcherBase_ptr TypedMatcherBase::Create (QVariant::Type type)
	{
		switch (type)
		{
		case QVariant::Int:
			return TypedMatcherBase_ptr (new IntMatcher ());
		case QVariant::String:
			return TypedMatcherBase_ptr (new StringMatcher ());
		case QVariant::StringList:
			return TypedMatcherBase_ptr (new StringListMatcher ());
		default:
			qWarning () << Q_FUNC_INFO
					<< "unknown type"
					<< type;
			return TypedMatcherBase_ptr ();
		}
	}

	StringLikeMatcher::StringLikeMatcher ()
	: Contains_ (true)
	{
	}

	QVariantMap StringLikeMatcher::Save () const
	{
		QVariantMap result;
		result ["Rx"] = Rx_;
		result ["Cont"] = Contains_;
		return result;
	}

	void StringLikeMatcher::Load (const QVariantMap& map)
	{
		Rx_ = map ["Rx"].toRegExp ();
		Contains_ = map ["Cont"].toBool ();
	}

	QWidget* StringLikeMatcher::GetConfigWidget ()
	{
		if (!CW_)
		{
			CW_ = new QWidget ();
			Ui_.reset (new Ui::StringLikeMatcherConfigWidget ());
			Ui_->setupUi (CW_);
		}

		Ui_->ContainsBox_->setCurrentIndex (Contains_ ? 0 : 1);
		Ui_->RegexpEditor_->setText (Rx_.pattern ());
		int rxIdx = 0;
		switch (Rx_.patternSyntax ())
		{
		case QRegExp::Wildcard:
			rxIdx = 1;
			break;
		case QRegExp::RegExp:
			rxIdx = 2;
			break;
		case QRegExp::FixedString:
		default:
			rxIdx = 0;
			break;
		}
		Ui_->RegexType_->setCurrentIndex (rxIdx);

		return CW_;
	}

	void StringLikeMatcher::SyncToWidget ()
	{
		if (!CW_)
		{
			qWarning () << Q_FUNC_INFO
					<< "called with null CW";
			return;
		}

		Contains_ = Ui_->ContainsBox_->currentIndex () == 0;

		QRegExp::PatternSyntax pattern = QRegExp::FixedString;
		switch (Ui_->RegexType_->currentIndex ())
		{
		case 0:
			break;
		case 1:
			pattern = QRegExp::Wildcard;
			break;
		case 2:
			pattern = QRegExp::RegExp;
			break;
		default:
			qWarning () << Q_FUNC_INFO
					<< "unknown regexp type"
					<< Ui_->RegexType_->currentIndex ();
			break;
		}

		Rx_ = QRegExp (Ui_->RegexpEditor_->text (),
				Qt::CaseInsensitive, pattern);
	}

	bool StringMatcher::Match (const QVariant& var) const
	{
		if (!var.canConvert<QString> ())
			return false;

		bool res = Rx_.indexIn (var.toString ()) != -1;
		if (!Contains_)
			res = !res;
		return res;
	}

	QString StringMatcher::GetHRDescription () const
	{
		const QString& p = Rx_.pattern ();
		return Contains_ ?
				QObject::tr ("contains pattern `%1`").arg (p) :
				QObject::tr ("doesn't contain pattern `%1`").arg (p);
	}

	bool StringListMatcher::Match (const QVariant& var) const
	{
		if (!var.canConvert<QStringList> ())
			return false;

		bool res = var.toStringList ().indexOf (Rx_) == -1;
		if (!Contains_)
			res = !res;
		return res;
	}

	QString StringListMatcher::GetHRDescription () const
	{
		const QString& p = Rx_.pattern ();
		return Contains_ ?
				QObject::tr ("contains element matching %1").arg (p) :
				QObject::tr ("doesn't contain element matching %1").arg (p);
	}

	IntMatcher::IntMatcher ()
	: Boundary_ (0)
	{
		Ops2pos_ [OGreater] = 0;
		Ops2pos_ [static_cast<Operations> (OEqual | OGreater)] = 1;
		Ops2pos_ [OEqual] = 2;
		Ops2pos_ [static_cast<Operations> (OEqual | OLess)] = 3;
		Ops2pos_ [OLess] = 4;
	}

	QVariantMap IntMatcher::Save () const
	{
		QVariantMap result;
		result ["Bd"] = Boundary_;
		result ["Ops"] = static_cast<quint16> (Ops_);
		return result;
	}

	void IntMatcher::Load (const QVariantMap& map)
	{
		Boundary_ = map ["Bd"].toInt ();
		Ops_ = static_cast<Operations> (map ["Ops"].value<quint16> ());
	}

	bool IntMatcher::Match (const QVariant& var) const
	{
		if (!var.canConvert<int> ())
			return false;

		const int val = var.toInt ();

		if ((Ops_ & OEqual) && val == Boundary_)
			return true;
		if ((Ops_ & OGreater) && val > Boundary_)
			return true;
		if ((Ops_ & OLess) && val < Boundary_)
			return true;

		return false;
	}

	QString IntMatcher::GetHRDescription () const
	{
		QString op;
		if ((Ops_ & OGreater))
			op += ">";
		if ((Ops_ & OLess))
			op += "<";
		if ((Ops_ & OEqual))
			op += "=";

		return QObject::tr ("is %1 then %2")
				.arg (op)
				.arg (Boundary_);
	}

	QWidget* IntMatcher::GetConfigWidget ()
	{
		if (!CW_)
		{
			CW_ = new QWidget ();
			Ui_.reset (new Ui::IntMatcherConfigWidget ());
			Ui_->setupUi (CW_);
		}

		Ui_->Boundary_->setValue (Boundary_);
		Ui_->OpType_->setCurrentIndex (Ops2pos_ [Ops_]);

		return CW_;
	}

	void IntMatcher::SyncToWidget ()
	{
		if (!CW_)
		{
			qWarning () << Q_FUNC_INFO
					<< "called with null CW";
			return;
		}

		Boundary_  = Ui_->Boundary_->value ();
		Ops_ = Ops2pos_.key (Ui_->OpType_->currentIndex ());
	}
}
}
