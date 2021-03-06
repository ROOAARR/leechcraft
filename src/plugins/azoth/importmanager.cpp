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

#include "importmanager.h"
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/ipluginsmanager.h>
#include "interfaces/azoth/iaccount.h"
#include "interfaces/azoth/isupportimport.h"
#include "interfaces/azoth/ihistoryplugin.h"
#include "core.h"
#include "accounthandlerchooserdialog.h"

namespace LeechCraft
{
namespace Azoth
{
	ImportManager::ImportManager (QObject *parent)
	: QObject (parent)
	{
	}

	void ImportManager::HandleAccountImport (Entity e)
	{
		const QVariantMap& map = e.Additional_ ["AccountData"].toMap ();
		const QString& protoId = map ["Protocol"].toString ();
		if (protoId.isEmpty ())
		{
			qWarning () << Q_FUNC_INFO
					<< "empty protocol id"
					<< map;
			return;
		}

		Q_FOREACH (IProtocol *proto, Core::Instance ().GetProtocols ())
		{
			ISupportImport *isi = qobject_cast<ISupportImport*> (proto->GetQObject ());
			if (!isi || isi->GetImportProtocolID () != protoId)
				continue;

			isi->ImportAccount (map);
			break;
		}
	}

	void ImportManager::HandleHistoryImport (Entity e)
	{
		qDebug () << Q_FUNC_INFO;
		const auto& histories = Core::Instance ().GetProxy ()->
				GetPluginsManager ()->GetAllCastableTo<IHistoryPlugin*> ();
		if (histories.isEmpty ())
		{
			qWarning () << Q_FUNC_INFO
					<< "no history plugin is present, aborting";
			return;
		}

		IAccount *acc = GetAccountID (e);
		if (!acc)
			return;

		ISupportImport *isi = qobject_cast<ISupportImport*> (acc->GetParentProtocol ());

		QHash<QString, QString> entryIDcache;

		QVariantList history;
		Q_FOREACH (Entity qe, EntityQueues_.take (e.Additional_ ["AccountID"].toString ()))
			history.append (qe.Additional_ ["History"].toList ());
		qDebug () << history.size ();
		Q_FOREACH (const QVariant& lineVar, history)
		{
			QVariantMap histMap = lineVar.toMap ();

			const QString& origID = histMap ["EntryID"].toString ();
			if (entryIDcache.contains (origID))
				histMap ["EntryID"] = entryIDcache [origID];
			else
			{
				const QString& realID = isi->GetEntryID (origID, acc->GetQObject ());
				entryIDcache [origID] = realID;
				histMap ["EntryID"] = realID;
			}

			if (histMap ["VisibleName"].toString ().isEmpty ())
				histMap ["VisibleName"] = origID;

			histMap ["AccountID"] = acc->GetAccountID ();

			if (histMap ["MessageType"] == "chat")
				histMap ["MessageType"] = static_cast<int> (IMessage::MTChatMessage);
			else if (histMap ["MessageType"] == "muc")
				histMap ["MessageType"] = static_cast<int> (IMessage::MTMUCMessage);
			else if (histMap ["MessageType"] == "event")
				histMap ["MessageType"] = static_cast<int> (IMessage::MTEventMessage);

			Q_FOREACH (IHistoryPlugin *plugin, histories)
				plugin->AddRawMessage (histMap);
		}
	}

	IAccount* ImportManager::GetAccountID (Entity e)
	{
		const QString& accName = e.Additional_ ["AccountName"].toString ();

		auto accs = Core::Instance ().GetAccounts ([] (IProtocol *proto)
				{ return qobject_cast<ISupportImport*> (proto->GetQObject ()); });
		IAccount *acc = 0;
		Q_FOREACH (acc, accs)
			if (acc->GetAccountName () == accName)
				return acc;

		const QString& impId = e.Additional_ ["AccountID"].toString ();

		EntityQueues_ [impId] << e;
		if (EntityQueues_ [impId].size () > 1)
			return 0;

		if (AccID2OurID_.contains (impId))
			return AccID2OurID_ [impId];

		QObjectList accObjs;
		Q_FOREACH (IAccount *ia, accs)
			accObjs << ia->GetQObject ();
		AccountHandlerChooserDialog dia (accObjs,
				tr ("Select account to import history from %1 into:").arg (accName));
		if (dia.exec () != QDialog::Accepted)
			return 0;

		acc = qobject_cast<IAccount*> (dia.GetSelectedAccount ());
		AccID2OurID_ [impId] = acc;
		return acc;
	}
}
}
