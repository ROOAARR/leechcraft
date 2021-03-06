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

#include "searcherslist.h"
#include <QInputDialog>
#include <util/tags/tagscompleter.h>
#include <interfaces/core/icoreproxy.h>
#include <interfaces/core/itagsmanager.h>
#include "core.h"
#include <util/util.h>

namespace LeechCraft
{
	namespace Plugins
	{
		namespace SeekThru
		{
			SearchersList::SearchersList (QWidget* parent)
			: QWidget (parent)
			{
				Ui_.setupUi (this);
				new Util::TagsCompleter (Ui_.Tags_, this);
				Ui_.Tags_->AddSelector ();
				Ui_.SearchersView_->setModel (&Core::Instance ());
				connect (Ui_.SearchersView_->selectionModel (),
						SIGNAL (currentRowChanged (const QModelIndex&, const QModelIndex&)),
						this,
						SLOT (handleCurrentChanged (const QModelIndex&)));

				auto menu = new QMenu (Ui_.ButtonAdd_);
				Ui_.ButtonAdd_->setMenu (menu);

				menu->addAction (tr ("From searchplugins.net..."),
						this,
						SLOT (handleOpenURL ()))->setData (QUrl ("http://searchplugins.net"));
			}

			void SearchersList::handleCurrentChanged (const QModelIndex& current)
			{
				Ui_.ButtonRemove_->setEnabled (current.isValid ());
				Ui_.InfoBox_->setEnabled (current.isValid ());

				Current_ = current;

				QString description = current.data (Core::RoleDescription).toString ();
				if (description.isEmpty ())
					Ui_.Description_->setText (tr ("No description"));
				else
					Ui_.Description_->setText (description);

				QString longName = current.data (Core::RoleLongName).toString ();
				if (longName.isEmpty ())
					Ui_.LongName_->setText (tr ("No long name"));
				else
					Ui_.LongName_->setText (longName);

				QStringList tags = current.data (Core::RoleTags).toStringList ();
				Ui_.Tags_->setText (Core::Instance ().GetProxy ()->
						GetTagsManager ()->Join (tags));

				QString contact = current.data (Core::RoleContact).toString ();
				if (contact.isEmpty ())
					Ui_.Contact_->setText (tr ("No contacts information"));
				else
					Ui_.Contact_->setText (contact);

				QString developer = current.data (Core::RoleDeveloper).toString ();
				if (developer.isEmpty ())
					Ui_.Developer_->setText (tr ("No developer information"));
				else
					Ui_.Developer_->setText (developer);

				QString attribution = current.data (Core::RoleAttribution).toString ();
				if (attribution.isEmpty ())
					Ui_.Attribution_->setText (tr ("No attribution information"));
				else
					Ui_.Attribution_->setText (attribution);

				QString right = current.data (Core::RoleRight).toString ();
				if (right.isEmpty ())
					Ui_.Right_->setText (tr ("No right information"));
				else
					Ui_.Right_->setText (right);

				bool adult = current.data (Core::RoleAdult).toBool ();
				Ui_.Adult_->setText (adult ? tr ("Yes") : tr ("No"));

				QStringList languages = current.data (Core::RoleLanguages).toStringList ();
			   	Ui_.Languages_->addItems (languages);
			}

			void SearchersList::on_ButtonAdd__released ()
			{
				QString url = QInputDialog::getText (this,
						tr ("Adding a new searcher"),
						tr ("Enter the URL of the OpenSearch description"));

				if (url.isEmpty ())
					return;

				Core::Instance ().Add (url);
			}

			void SearchersList::on_ButtonRemove__released ()
			{
				Core::Instance ().Remove (Ui_.SearchersView_->selectionModel ()->currentIndex ());
			}

			void SearchersList::on_Tags__editingFinished ()
			{
				Core::Instance ().SetTags (Current_,
						Core::Instance ().GetProxy ()->
							GetTagsManager ()->Split (Ui_.Tags_->text ()));
			}

			void SearchersList::handleOpenURL ()
			{
				auto act = qobject_cast<QAction*> (sender ());

				const auto& e = Util::MakeEntity (act->data (),
						QString (), FromUserInitiated | OnlyHandle);
				emit gotEntity (e);
			}
		};
	};
};

