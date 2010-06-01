/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2010  Georg Rudoy
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include "pluginmanager.h"
#include <QDir>
#include <QtDebug>
#include <qross/core/manager.h>
#include "wrapperobject.h"

namespace LeechCraft
{
	namespace Plugins
	{
		namespace Qrosp
		{
			PluginManager::PluginManager ()
			{
				qDebug () << Q_FUNC_INFO
						<< "interpreters:"
						<< Qross::Manager::self ().interpreters ();

				QMap<QString, QStringList> plugins = FindPlugins ();
				qDebug () << Q_FUNC_INFO
						<< "found"
						<< plugins;

				Q_FOREACH (QString type, plugins.keys ())
					Q_FOREACH (QString path, plugins [type])
						Wrappers_ << new WrapperObject (type, path);
			}

			PluginManager& PluginManager::Instance ()
			{
				static PluginManager pm;
				return pm;
			}

			QList<QObject*> PluginManager::GetPlugins ()
			{
				return Wrappers_;
			}

			QMap<QString, QStringList> PluginManager::FindPlugins ()
			{
				QMap<QString, QStringList> knownExtensions;
				knownExtensions ["javascript"] << "*.es" << "*.js" << "*.qs";
				knownExtensions ["python"] << "*.py";
				knownExtensions ["ruby"] << "*.rb";

				QMap<QString, QStringList> result;

				struct Collector
				{
					const QStringList& Extensions_;

					Collector (const QStringList& exts)
					: Extensions_ (exts)
					{
					}

					QFileInfoList operator() (const QString& path)
					{
						QFileInfoList list;
						QDir dir = QDir::home ();
						if (dir.cd (path))
							list = dir.entryInfoList (Extensions_,
									QDir::Files |
										QDir::NoDotAndDotDot |
										QDir::Readable,
									QDir::Name);
						return list;
					}
				};

				QDir dir = QDir::home ();
				if (!dir.cd (".leechcraft/plugins/scriptable"))
					qWarning () << Q_FUNC_INFO
							<< "unable to cd into ~/.leechcraft/plugins/scriptable";
				else
					Q_FOREACH (QFileInfo dirInfo,
							dir.entryInfoList (QDir::Dirs |
									QDir::NoDotAndDotDot |
									QDir::Readable))
					{
						if (!dirInfo.isReadable ())
							continue;
						QFileInfoList list;
						QString type = dirInfo.fileName ();
						QStringList exts = knownExtensions.value (type, QStringList ("*.*"));
						list += Collector (exts) (dirInfo.absoluteFilePath ());

						Q_FOREACH (QFileInfo info, list)
							result [type] += info.absoluteFilePath ();
					}

				return result;
			}
		};
	};
};
