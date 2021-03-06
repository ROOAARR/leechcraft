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

#include "selfavatarfetcher.h"
#include <QTimer>
#include <QStringList>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QtDebug>
#include "core.h"

namespace LeechCraft
{
namespace Azoth
{
namespace Vader
{
	SelfAvatarFetcher::SelfAvatarFetcher (QObject *parent)
	: QObject (parent)
	, Timer_ (new QTimer (this))
	{
		connect (Timer_,
				SIGNAL (timeout ()),
				this,
				SLOT (refetch ()));
		Timer_->setInterval (120 * 60 * 1000);
	}

	void SelfAvatarFetcher::Restart (const QString& full)
	{
		const QStringList& split = full.split ('@', QString::SkipEmptyParts);

		Name_ = split.value (0);
		Domain_ = split.value (1);
		if (Domain_.endsWith (".ru"))
			Domain_.chop (3);

		if (Timer_->isActive ())
			Timer_->stop ();
		Timer_->start ();

		QTimer::singleShot (2000,
				this,
				SLOT (refetch ()));
	}

	QUrl SelfAvatarFetcher::GetReqURL () const
	{
		QString urlStr = "http://obraz.foto.mail.ru/" + Domain_ + "/" + Name_ + "/_mrimavatarsmall";
		return QUrl (urlStr);
	}

	void SelfAvatarFetcher::refetch ()
	{
		auto nam = Core::Instance ().GetCoreProxy ()->GetNetworkAccessManager ();
		QNetworkReply *reply = nam->head (QNetworkRequest (GetReqURL ()));
		connect (reply,
				SIGNAL (finished ()),
				this,
				SLOT (handleHeadFinished ()));
	}

	void SelfAvatarFetcher::handleHeadFinished ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();
		if (reply->error () == QNetworkReply::ContentNotFoundError)
		{
			qDebug () << Q_FUNC_INFO
					<< "avatar not found for"
					<< Name_
					<< Domain_;
			return;
		}

		const auto& dt = reply->header (QNetworkRequest::LastModifiedHeader).toDateTime ();
		if (dt <= PreviousDateTime_)
			return;

		PreviousDateTime_ = dt;

		auto nam = Core::Instance ().GetCoreProxy ()->GetNetworkAccessManager ();
		QNetworkReply *getReply = nam->get (QNetworkRequest (GetReqURL ()));
		connect (getReply,
				SIGNAL (finished ()),
				this,
				SLOT (handleGetFinished ()));
	}

	void SelfAvatarFetcher::handleGetFinished ()
	{
		auto reply = qobject_cast<QNetworkReply*> (sender ());
		reply->deleteLater ();
		const QImage& image = QImage::fromData (reply->readAll ());
		emit gotImage (image);
	}
}
}
}
