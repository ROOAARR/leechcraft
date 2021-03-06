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

#include "quarkmanager.h"
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QDeclarativeImageProvider>
#include <QStandardItem>
#include <QDialog>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QFile>
#include <QtDebug>
#include <QFileInfo>
#include <QDir>
#include <qjson/parser.h>
#include <interfaces/iquarkcomponentprovider.h>
#include "viewmanager.h"
#include "sbview.h"
#include "quarksettingsmanager.h"

namespace LeechCraft
{
namespace SB2
{
	const int IconSize = 32;

	namespace
	{
		class ImageProvProxy : public QDeclarativeImageProvider
		{
			QDeclarativeImageProvider *Wrapped_;
		public:
			ImageProvProxy (QDeclarativeImageProvider *other)
			: QDeclarativeImageProvider (other->imageType ())
			, Wrapped_ (other)
			{
			}

			QImage requestImage (const QString& id, QSize *size, const QSize& requestedSize)
			{
				return Wrapped_->requestImage (id, size, requestedSize);
			}

			QPixmap requestPixmap (const QString& id, QSize *size, const QSize& requestedSize)
			{
				return Wrapped_->requestPixmap (id, size, requestedSize);
			}
		};
	}

	QuarkManager::QuarkManager (QuarkComponent_ptr comp,
			ViewManager *manager, ICoreProxy_ptr proxy)
	: QObject (manager)
	, ViewMgr_ (manager)
	, Proxy_ (proxy)
	, Component_ (comp)
	, URL_ (comp->Url_)
	, SettingsManager_ (0)
	, ID_ (QFileInfo (URL_.path ()).fileName ())
	, Name_ (ID_)
	, Icon_ (proxy->GetIcon ("applications-science"))
	{
		ParseManifest ();

		if (!ViewMgr_)
			return;

		qDebug () << Q_FUNC_INFO << "adding" << comp->Url_;
		auto ctx = manager->GetView ()->rootContext ();
		for (const auto& pair : comp->StaticProps_)
			ctx->setContextProperty (pair.first, pair.second);
		for (const auto& pair : comp->DynamicProps_)
			ctx->setContextProperty (pair.first, pair.second);
		for (const auto& pair : comp->ContextProps_)
			ctx->setContextProperty (pair.first, pair.second);

		auto engine = manager->GetView ()->engine ();
		for (const auto& pair : comp->ImageProviders_)
		{
			if (auto old = engine->imageProvider (pair.first))
			{
				engine->removeImageProvider (pair.first);
				delete old;
			}
			engine->addImageProvider (pair.first, new ImageProvProxy (pair.second));
		}

		CreateSettings ();
	}

	QString QuarkManager::GetID () const
	{
		return ID_;
	}

	QString QuarkManager::GetName () const
	{
		return Name_;
	}

	QIcon QuarkManager::GetIcon () const
	{
		return Icon_;
	}

	QString QuarkManager::GetDescription () const
	{
		return Description_;
	}

	bool QuarkManager::IsValidArea () const
	{
		return Areas_.isEmpty () || Areas_.contains ("panel");
	}

	bool QuarkManager::HasSettings () const
	{
		return SettingsManager_;
	}

	void QuarkManager::ShowSettings ()
	{
		if (!HasSettings ())
			return;

		QDialog dia;
		const auto& settingsTitle = Name_.isEmpty () ?
				tr ("Settings") :
				tr ("Settings for %1").arg (Name_);
		dia.setWindowTitle (settingsTitle);

		dia.setLayout (new QVBoxLayout ());
		dia.layout ()->addWidget (XSD_.get ());

		auto box = new QDialogButtonBox (QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		connect (box,
				SIGNAL (accepted ()),
				&dia,
				SLOT (accept ()));
		connect (box,
				SIGNAL (rejected ()),
				&dia,
				SLOT (reject ()));
		connect (box,
				SIGNAL (accepted ()),
				XSD_.get (),
				SLOT (accept ()));
		connect (box,
				SIGNAL (rejected ()),
				XSD_.get (),
				SLOT (reject ()));
		dia.layout ()->addWidget (box);

		dia.exec ();
		XSD_->setParent (0);
	}

	QString QuarkManager::GetSuffixedName (const QString& suffix) const
	{
		if (!URL_.isLocalFile ())
			return QString ();

		const auto& localName = URL_.toLocalFile ();
		const auto& suffixed = localName + suffix;
		if (!QFile::exists (suffixed))
			return QString ();

		return suffixed;
	}

	void QuarkManager::ParseManifest ()
	{
		const auto& manifestName = GetSuffixedName (".manifest");
		if (manifestName.isEmpty ())
			return;

		QFile file (manifestName);
		if (!file.open (QIODevice::ReadOnly))
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to open manifest file"
					<< file.errorString ();
			return;
		}

		QJson::Parser parser;
		bool ok = false;
		const auto& varMap = parser.parse (&file, &ok).toMap ();
		if (!ok)
		{
			qWarning () << Q_FUNC_INFO
					<< "failed to parse"
					<< manifestName
					<< parser.errorLine ()
					<< parser.errorString ();
			return;
		}

		Name_ = varMap ["quarkName"].toString ();
		Areas_ = varMap ["areas"].toStringList ();

		Description_ = varMap ["description"].toString ();

		if (varMap.contains ("quarkID"))
			ID_ = varMap ["quarkID"].toString ();

		if (varMap.contains ("icon"))
		{
			const auto& iconName = varMap ["icon"].toString ();

			TryFullImage (iconName) || TryTheme (iconName) || TryLC (iconName);
		}
	}

	bool QuarkManager::TryFullImage (const QString& iconName)
	{
		const auto& dirName = QFileInfo (URL_.toLocalFile ()).absoluteDir ().path ();
		const auto& fullName = dirName + '/' + iconName;

		const QPixmap px (fullName);
		if (px.isNull ())
			return false;

		Icon_ = QIcon ();
		Icon_.addPixmap (px);
		return true;
	}

	bool QuarkManager::TryTheme (const QString& iconName)
	{
		const auto& icon = Proxy_->GetIcon (iconName);
		const auto& px = icon.pixmap (IconSize, IconSize);
		if (px.isNull ())
			return false;

		Icon_ = icon;
		return true;
	}

	bool QuarkManager::TryLC (const QString& iconName)
	{
		if (iconName != "leechcraft")
			return false;

		Icon_ = QIcon ();
		Icon_.addFile ("lcicons:/resources/images/leechcraft.svg");
		return true;
	}

	void QuarkManager::CreateSettings ()
	{
		const auto& settingsName = GetSuffixedName (".settings");
		if (settingsName.isEmpty ())
			return;

		XSD_.reset (new Util::XmlSettingsDialog);
		SettingsManager_ = new QuarkSettingsManager (URL_, ViewMgr_->GetView ()->rootContext ());
		XSD_->RegisterObject (SettingsManager_, settingsName);
	}
}
}
