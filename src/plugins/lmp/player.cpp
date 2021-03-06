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

#include "player.h"
#include <algorithm>
#include <random>
#include <QStandardItemModel>
#include <QFileInfo>
#include <QDir>
#include <QMimeData>
#include <QUrl>
#include <QtConcurrentRun>
#include <QApplication>
#include <phonon/mediaobject.h>
#include <phonon/audiooutput.h>
#include <util/util.h>
#include "core.h"
#include "mediainfo.h"
#include "localfileresolver.h"
#include "util.h"
#include "localcollection.h"
#include "playlistmanager.h"
#include "staticplaylistmanager.h"
#include "xmlsettingsmanager.h"
#include "playlistparsers/playlistfactory.h"

#if defined(Q_OS_WIN32)
	#ifdef GetQObject
		#undef GetQObject
	#endif
#endif

Q_DECLARE_METATYPE (Phonon::MediaSource);
Q_DECLARE_METATYPE (QList<Phonon::MediaSource>);

namespace Phonon
{
	uint qHash (const Phonon::MediaSource& src)
	{
		uint hash = 0;
		switch (src.type ())
		{
		case Phonon::MediaSource::LocalFile:
			hash = qHash (src.fileName ());
			break;
		case Phonon::MediaSource::Url:
			hash = qHash (src.url ());
			break;
		case Phonon::MediaSource::Disc:
			hash = src.discType ();
			break;
		case Phonon::MediaSource::Stream:
			hash = qHash (src.deviceName ());
			break;
		default:
			hash = 0;
			break;
		}
		return hash << src.type ();
	}
}

namespace LeechCraft
{
namespace LMP
{
	namespace
	{
		class PlaylistModel : public QStandardItemModel
		{
			Player *Player_;
		public:
			PlaylistModel (Player *parent)
			: QStandardItemModel (parent)
			, Player_ (parent)
			{
				setSupportedDragActions (Qt::CopyAction | Qt::MoveAction);
			}

			QStringList mimeTypes () const
			{
				return QStringList ("text/uri-list");
			}

			QMimeData* mimeData (const QModelIndexList& indexes) const
			{
				QList<QUrl> urls;
				Q_FOREACH (const auto& index, indexes)
				{
					const auto& sources = Player_->GetIndexSources (index);
					std::transform (sources.begin (), sources.end (), std::back_inserter (urls),
							[] (decltype (sources.front ()) source)
							{
								return source.type () == Phonon::MediaSource::LocalFile ?
										QUrl::fromLocalFile (source.fileName ()) :
										source.url ();
							});
				}
				urls.removeAll (QUrl ());

				QMimeData *result = new QMimeData;
				result->setUrls (urls);
				return result;
			}

			bool dropMimeData (const QMimeData *data, Qt::DropAction action, int row, int, const QModelIndex& parent)
			{
				if (action == Qt::IgnoreAction)
					return true;

				if (!data->hasUrls ())
					return false;

				const auto& urls = data->urls ();
				QList<Phonon::MediaSource> sources;
				for (const auto& url : urls)
				{
					if (url.scheme () != "file")
					{
						sources << Phonon::MediaSource (url);
						continue;
					}

					const auto& localPath = url.toLocalFile ();
					if (QFileInfo (localPath).isFile ())
					{
						bool playlistHandled = false;
						if (auto f = MakePlaylistParser (localPath))
						{
							const auto& playlistSrcs = f (localPath);
							if (!playlistSrcs.isEmpty ())
							{
								playlistHandled = true;
								sources += playlistSrcs;
							}
						}

						if (!playlistHandled)
							sources << Phonon::MediaSource (localPath);
						continue;
					}

					for (const auto& path : RecIterate (localPath, true))
						sources << Phonon::MediaSource (path);
				}

				auto afterIdx = row >= 0 ?
						parent.child (row, 0) :
						parent;
				const auto& firstSrc = afterIdx.isValid () ?
						Player_->GetIndexSources (afterIdx).value (0) :
						Phonon::MediaSource ();

				auto existingQueue = Player_->GetQueue ();
				if (action == Qt::MoveAction)
					Q_FOREACH (const auto& src, sources)
					{
						auto pred = [&src] (decltype (existingQueue.front ()) item) -> bool
						{
							if (src.type () != item.type ())
								return false;
							if (src.type () == Phonon::MediaSource::LocalFile)
								return src.fileName () == item.fileName ();
							if (src.type () == Phonon::MediaSource::Url)
								return src.url () == item.url ();
							return false;
						};
						auto remPos = std::remove_if (existingQueue.begin (), existingQueue.end (), pred);
						existingQueue.erase (remPos, existingQueue.end ());
					}

				auto pos = std::find (existingQueue.begin (), existingQueue.end (), firstSrc);
				if (pos == existingQueue.end ())
					existingQueue << sources;
				else
				{
					for (const auto& src : sources)
						pos = existingQueue.insert (pos, src) + 1;
				}

				Player_->ReplaceQueue (existingQueue);
				return true;
			}

			Qt::DropActions supportedDropActions () const
			{
				return Qt::CopyAction | Qt::MoveAction;
			}
		};
	}

	Player::Sorter::Sorter ()
	{
		Criteria_ << SortingCriteria::Artist
				<< SortingCriteria::Year
				<< SortingCriteria::Album
				<< SortingCriteria::TrackNumber;
	}

	bool Player::Sorter::operator() (const MediaInfo& left, const MediaInfo& right) const
	{
		Q_FOREACH (auto crit, Criteria_)
		{
			switch (crit)
			{
			case SortingCriteria::Artist:
				if (left.Artist_ != right.Artist_)
					return left.Artist_ < right.Artist_;
				break;
			case SortingCriteria::Year:
				if (left.Year_ != right.Year_)
					return left.Year_ < right.Year_;
				break;
			case SortingCriteria::Album:
				if (left.Album_ != right.Album_)
					return left.Album_ < right.Album_;
				break;
			case SortingCriteria::TrackNumber:
				if (left.TrackNumber_ != right.TrackNumber_)
					return left.TrackNumber_ < right.TrackNumber_;
				break;
			case SortingCriteria::TrackTitle:
				if (left.Title_ != right.Title_)
					return left.Title_ < right.Title_;
				break;
			case SortingCriteria::DirectoryPath:
			{
				const auto& leftPath = QFileInfo (left.LocalPath_).dir ().absolutePath ();
				const auto& rightPath = QFileInfo (right.LocalPath_).dir ().absolutePath ();
				if (leftPath != rightPath)
					return QString::localeAwareCompare (leftPath, rightPath) < 0;
				break;
			}
			case SortingCriteria::FileName:
			{
				const auto& leftPath = QFileInfo (left.LocalPath_).fileName ();
				const auto& rightPath = QFileInfo (right.LocalPath_).fileName ();
				if (leftPath != rightPath)
					return QString::localeAwareCompare (leftPath, rightPath) < 0;
				break;
			}
			}
		}

		return left.LocalPath_ < right.LocalPath_;
	}

	Player::Player (QObject *parent)
	: QObject (parent)
	, PlaylistModel_ (new PlaylistModel (this))
	, Source_ (new Phonon::MediaObject (this))
	, Output_ (new Phonon::AudioOutput (Phonon::MusicCategory, this))
	, Path_ (Phonon::createPath (Source_, Output_))
	, RadioItem_ (0)
	, PlayMode_ (PlayMode::Sequential)
	{
		qRegisterMetaType<QList<Phonon::MediaSource>> ("QList<Phonon::MediaSource>");
		qRegisterMetaType<StringPair_t> ("StringPair_t");

		connect (Source_,
				SIGNAL (currentSourceChanged (Phonon::MediaSource)),
				this,
				SLOT (handleCurrentSourceChanged (Phonon::MediaSource)));
		connect (Source_,
				SIGNAL (aboutToFinish ()),
				this,
				SLOT (handleUpdateSourceQueue ()));
		Source_->setTickInterval (1000);
		Source_->setPrefinishMark (2000);

		XmlSettingsManager::Instance ().RegisterObject ("TransitionTime",
				this, "setTransitionTime");
		setTransitionTime ();

		XmlSettingsManager::Instance ().RegisterObject ("SingleTrackDisplayMask",
				this, "refillPlaylist");

		const auto& criteriaVar = XmlSettingsManager::Instance ().property ("SortingCriteria");
		if (!criteriaVar.isNull ())
			Sorter_.Criteria_ = LoadCriteria (criteriaVar);

		connect (Source_,
				SIGNAL (finished ()),
				this,
				SLOT (handlePlaybackFinished ()));
		connect (Source_,
				SIGNAL (stateChanged (Phonon::State, Phonon::State)),
				this,
				SLOT (handleStateChanged (Phonon::State)));

		connect (Source_,
				SIGNAL (metaDataChanged ()),
				this,
				SLOT (handleMetadata ()));
		connect (Source_,
				SIGNAL (bufferStatus (int)),
				this,
				SIGNAL (bufferStatusChanged (int)));

		auto collection = Core::Instance ().GetLocalCollection ();
		if (collection->IsReady ())
			restorePlaylist ();
		else
			connect (collection,
					SIGNAL (collectionReady ()),
					this,
					SLOT (restorePlaylist ()));
	}

	QAbstractItemModel* Player::GetPlaylistModel () const
	{
		return PlaylistModel_;
	}

	Phonon::MediaObject* Player::GetSourceObject () const
	{
		return Source_;
	}

	Phonon::AudioOutput* Player::GetAudioOutput () const
	{
		return Output_;
	}

	Player::PlayMode Player::GetPlayMode () const
	{
		return PlayMode_;
	}

	void Player::SetPlayMode (Player::PlayMode playMode)
	{
		if (PlayMode_ == playMode)
			return;

		PlayMode_ = playMode;
		emit playModeChanged (PlayMode_);
	}

	QList<SortingCriteria> Player::GetSortingCriteria () const
	{
		return Sorter_.Criteria_;
	}

	void Player::SetSortingCriteria (const QList<SortingCriteria>& criteria)
	{
		Sorter_.Criteria_ = criteria;

		AddToPlaylistModel (QList<Phonon::MediaSource> (), true);

		XmlSettingsManager::Instance ().setProperty ("SortingCriteria", SaveCriteria (criteria));
	}

	namespace
	{
		QList<Phonon::MediaSource> FileToSource (const QString& file)
		{
			auto parser = MakePlaylistParser (file);
			if (parser)
			{
				const auto& list = parser (file);
				if (!list.isEmpty ())
					return list;
			}

			return { Phonon::MediaSource (file) };
		}
	}

	void Player::PrepareURLInfo (const QUrl& url, const MediaInfo& info)
	{
		Url2Info_ [url] = info;
	}

	void Player::Enqueue (const QStringList& paths, bool sort)
	{
		QList<Phonon::MediaSource> sources;
		std::for_each (paths.begin (), paths.end (),
				[&sources] (decltype (paths.front ()) path)
					{ sources += FileToSource (path); });
		Enqueue (sources, sort);
	}

	void Player::Enqueue (const QList<Phonon::MediaSource>& sources, bool sort)
	{
		AddToPlaylistModel (sources, sort);
	}

	void Player::ReplaceQueue (const QList<Phonon::MediaSource>& queue, bool sort)
	{
		PlaylistModel_->clear ();
		Items_.clear ();
		AlbumRoots_.clear ();
		CurrentQueue_.clear ();

		AddToPlaylistModel (queue, sort);
	}

	QList<Phonon::MediaSource> Player::GetQueue () const
	{
		return CurrentQueue_;
	}

	QList<Phonon::MediaSource> Player::GetIndexSources (const QModelIndex& index) const
	{
		QList<Phonon::MediaSource> sources;
		if (index.data (Role::IsAlbum).toBool ())
			for (int i = 0; i < PlaylistModel_->rowCount (index); ++i)
				sources << PlaylistModel_->index (i, 0, index).data (Role::Source).value<Phonon::MediaSource> ();
		else
			sources << index.data (Role::Source).value<Phonon::MediaSource> ();
		return sources;
	}

	namespace
	{
		void IncAlbumLength (QStandardItem *albumItem, int length)
		{
			const int prevLength = albumItem->data (Player::Role::AlbumLength).toInt ();
			albumItem->setData (length + prevLength, Player::Role::AlbumLength);
		}
	}

	void Player::Dequeue (const QModelIndex& index)
	{
		if (!index.isValid ())
			return;

		Dequeue (GetIndexSources (index));
	}

	void Player::Dequeue (const QList<Phonon::MediaSource>& sources)
	{
		Q_FOREACH (const auto& source, sources)
		{
			Url2Info_.remove (source.url ());

			if (!CurrentQueue_.removeAll (source))
				continue;

			auto item = Items_.take (source);
			auto parent = item->parent ();
			if (parent)
			{
				if (parent->rowCount () == 1)
				{
					Q_FOREACH (const auto& key, AlbumRoots_.keys ())
					{
						auto& items = AlbumRoots_ [key];
						if (!items.contains (parent))
							continue;

						items.removeAll (parent);
						if (items.isEmpty ())
							AlbumRoots_.remove (key);
					}
					PlaylistModel_->removeRow (parent->row ());
				}
				else
				{
					const auto& info = item->data (Role::Info).value<MediaInfo> ();
					if (!info.LocalPath_.isEmpty ())
						IncAlbumLength (parent, -info.Length_);
					parent->removeRow (item->row ());
				}
			}
			else
				PlaylistModel_->removeRow (item->row ());
		}

		Core::Instance ().GetPlaylistManager ()->
				GetStaticManager ()->SetOnLoadPlaylist (CurrentQueue_);
	}

	void Player::SetStopAfter (const QModelIndex& index)
	{
		if (!index.isValid ())
			return;

		Phonon::MediaSource stopSource;
		if (index.data (Role::IsAlbum).toBool ())
			stopSource = PlaylistModel_->index (0, 0, index).data (Role::Source).value<Phonon::MediaSource> ();
		else
			stopSource = index.data (Role::Source).value<Phonon::MediaSource> ();

		if (CurrentStopSource_.type () != Phonon::MediaSource::Empty)
			Items_ [CurrentStopSource_]->setData (false, Role::IsStop);

		if (CurrentStopSource_ == stopSource)
			CurrentStopSource_ = Phonon::MediaSource ();
		else
		{
			CurrentStopSource_ = stopSource;
			Items_ [stopSource]->setData (true, Role::IsStop);
		}
	}

	void Player::SetRadioStation (Media::IRadioStation_ptr station)
	{
		clear ();

		CurrentStation_ = station;

		connect (CurrentStation_->GetQObject (),
				SIGNAL (gotError (const QString&)),
				this,
				SLOT (handleStationError (const QString&)));
		connect (CurrentStation_->GetQObject (),
				SIGNAL (gotNewStream (QUrl, Media::AudioInfo)),
				this,
				SLOT (handleRadioStream (QUrl, Media::AudioInfo)));
		connect (CurrentStation_->GetQObject (),
				SIGNAL (gotPlaylist (QString, QString)),
				this,
				SLOT (handleGotRadioPlaylist (QString, QString)));
		CurrentStation_->RequestNewStream ();

		auto radioName = station->GetRadioName ();
		if (radioName.isEmpty ())
			radioName = tr ("Radio");
		RadioItem_ = new QStandardItem (radioName);
		RadioItem_->setEditable (false);
		PlaylistModel_->appendRow (RadioItem_);
	}

	MediaInfo Player::GetCurrentMediaInfo () const
	{
		const auto& source = Source_->currentSource ();
		if (source.type () == Phonon::MediaSource::Empty)
			return MediaInfo ();

		auto info = GetMediaInfo (source);
		if (!info.LocalPath_.isEmpty ())
			return info;

		info = GetPhononMediaInfo ();
		return info;
	}

	QString Player::GetCurrentAAPath () const
	{
		const auto& info = GetCurrentMediaInfo ();
		auto coll = Core::Instance ().GetLocalCollection ();
		auto album = coll->GetAlbum (coll->FindAlbum (info.Artist_, info.Album_));
		return album ? album->CoverPath_ : QString ();;
	}

	MediaInfo Player::GetMediaInfo (const Phonon::MediaSource& source) const
	{
		return Items_.contains (source) ?
				Items_ [source]->data (Role::Info).value<MediaInfo> () :
				MediaInfo ();
	}

	MediaInfo Player::GetPhononMediaInfo () const
	{
		MediaInfo info;
		info.Artist_ = Source_->metaData (Phonon::ArtistMetaData).value (0);
		info.Album_ = Source_->metaData (Phonon::AlbumMetaData).value (0);
		info.Title_ = Source_->metaData (Phonon::TitleMetaData).value (0);
		info.Genres_ = Source_->metaData (Phonon::GenreMetaData);
		info.TrackNumber_ = Source_->metaData (Phonon::TracknumberMetaData).value (0).toInt ();
		info.Length_ = Source_->totalTime () / 1000;

		if (info.Artist_.isEmpty () && info.Title_.contains (" - "))
		{
			const auto& strs = info.Title_.split (" - ", QString::SkipEmptyParts);
			switch (strs.size ())
			{
			case 2:
				info.Artist_ = strs.value (0);
				info.Title_ = strs.value (1);
				break;
			case 3:
				info.Artist_ = strs.value (0);
				info.Album_ = strs.value (1);
				info.Title_ = strs.value (2);
				break;
			}
		}

		return info;
	}

	namespace
	{
		void FillItem (QStandardItem *item, const MediaInfo& info)
		{
			QString text;
			if (!info.IsUseless ())
			{
				text = XmlSettingsManager::Instance ()
						.property ("SingleTrackDisplayMask").toString ();

				text = PerformSubstitutions (text, info).simplified ();
				text.replace ("- -", "-");
				if (text.startsWith ("- "))
					text = text.mid (2);
				if (text.endsWith (" -"))
					text.chop (2);
			}
			else
				text = QFileInfo (info.LocalPath_).fileName ();

			item->setText (text);

			item->setData (QVariant::fromValue (info), Player::Role::Info);
		}

		QStandardItem* MakeAlbumItem (const MediaInfo& info)
		{
			auto albumItem = new QStandardItem (QString ("%1 - %2")
							.arg (info.Artist_, info.Album_));
			albumItem->setEditable (false);
			albumItem->setData (true, Player::Role::IsAlbum);
			albumItem->setData (QVariant::fromValue (info), Player::Role::Info);
			auto art = FindAlbumArt (info.LocalPath_);
			const int dim = 48;
			if (art.isNull ())
				art = QIcon::fromTheme ("media-optical").pixmap (dim, dim);
			else if (std::max (art.width (), art.height ()) > dim)
				art = art.scaled (dim, dim, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			albumItem->setData (art, Player::Role::AlbumArt);
			albumItem->setData (0, Player::Role::AlbumLength);
			return albumItem;
		}
	}

	namespace
	{
		QPair<Phonon::MediaSource, MediaInfo> PairResolve (const Phonon::MediaSource& source)
		{
			auto resolver = Core::Instance ().GetLocalFileResolver ();

			MediaInfo info;
			if (source.type () == Phonon::MediaSource::LocalFile)
				try
				{
					info = resolver->ResolveInfo (source.fileName ());
				}
				catch (...)
				{
					info.LocalPath_ = source.fileName ();
				}

			return qMakePair (source, info);
		}

		QList<QPair<Phonon::MediaSource, MediaInfo>> PairResolveAll (const QList<Phonon::MediaSource>& sources)
		{
			QList<QPair<Phonon::MediaSource, MediaInfo>> result;
			std::transform (sources.begin (), sources.end (), std::back_inserter (result), PairResolve);
			return result;
		}

		template<typename T>
		QList<QPair<Phonon::MediaSource, MediaInfo>> PairResolveSort (const QList<Phonon::MediaSource>& sources, T sorter, bool sort)
		{
			auto result = PairResolveAll (sources);

			if (sorter.Criteria_.isEmpty () || !sort)
				return result;

			std::sort (result.begin (), result.end (),
					[sorter] (decltype (result.at (0)) s1, decltype (result.at (0)) s2) -> bool
					{
						if (s1.first.type () != Phonon::MediaSource::LocalFile ||
							s2.first.type () != Phonon::MediaSource::LocalFile)
							return qHash (s1.first) < qHash (s2.first);

						return sorter (s1.second, s2.second);
					});

			return result;
		}
	}

	void Player::AddToPlaylistModel (QList<Phonon::MediaSource> sources, bool sort)
	{
		if (!CurrentQueue_.isEmpty ())
		{
			ReplaceQueue (CurrentQueue_ + sources, sort);
			return;
		}

		PlaylistModel_->setHorizontalHeaderLabels (QStringList (tr ("Playlist")));

		std::function<QList<QPair<Phonon::MediaSource, MediaInfo>> ()> worker =
				[this, sources, sort] ()
				{
					return PairResolveSort (sources, Sorter_, sort);
				};

		emit playerAvailable (false);

		auto watcher = new QFutureWatcher<QList<QPair<Phonon::MediaSource, MediaInfo>>> ();
		connect (watcher,
				SIGNAL (finished ()),
				this,
				SLOT (handleSorted ()));
		watcher->setFuture (QtConcurrent::run (worker));
	}

	bool Player::HandleCurrentStop (const Phonon::MediaSource& source)
	{
		if (source != CurrentStopSource_)
			return false;

		CurrentStopSource_ = Phonon::MediaSource ();
		Items_ [source]->setData (false, Role::IsStop);

		return true;
	}

	void Player::UnsetRadio ()
	{
		if (!CurrentStation_)
			return;

		if (RadioItem_)
			PlaylistModel_->removeRow (RadioItem_->row ());
		RadioItem_ = 0;

		CurrentStation_.reset ();
	}

	template<typename T>
	Phonon::MediaSource Player::GetRandomBy (QList<Phonon::MediaSource>::const_iterator pos,
			std::function<T (Phonon::MediaSource)> feature) const
	{
		auto randPos = [&feature] (const QList<Phonon::MediaSource>& sources) -> int
		{
			QHash<T, QList<int>> fVals;
			for (int i = 0; i < sources.size (); ++i)
				fVals [feature (sources.at (i))] << i;

			static std::random_device generator;
			std::uniform_int_distribution<int> dist (0, sources.size () - 1);
			auto fIdx = std::uniform_int_distribution<int> (0, fVals.size () - 1) (generator);

			auto fPos = fVals.begin ();
			std::advance (fPos, fIdx);
			const auto& positions = *fPos;
			if (positions.size () < 2)
				return positions [0];

			auto posIdx = std::uniform_int_distribution<int> (0, positions.size () - 1) (generator);
			return positions [posIdx];
		};
		auto rand = [&randPos] (const QList<Phonon::MediaSource>& sources)
			{ return sources.at (randPos (sources)); };

		if (pos == CurrentQueue_.end ())
			return rand (CurrentQueue_);

		const auto& current = feature (*pos);
		++pos;
		if (pos != CurrentQueue_.end () && feature (*pos) == current)
			return *pos;

		auto modifiedQueue = CurrentQueue_;
		auto endPos = std::remove_if (modifiedQueue.begin (), modifiedQueue.end (),
				[&current, &feature, this] (decltype (modifiedQueue.at (0)) source)
					{ return feature (source) == current; });
		modifiedQueue.erase (endPos, modifiedQueue.end ());
		if (modifiedQueue.isEmpty ())
			return rand (CurrentQueue_);

		pos = modifiedQueue.begin () + randPos (modifiedQueue);
		const auto& origFeature = feature (*pos);
		while (pos != modifiedQueue.begin ())
		{
			if (feature (*(pos - 1)) != origFeature)
				break;
			--pos;
		}
		return *pos;
	}

	Phonon::MediaSource Player::GetNextSource (const Phonon::MediaSource& current) const
	{
		if (CurrentQueue_.isEmpty ())
			return {};

		auto pos = std::find (CurrentQueue_.begin (), CurrentQueue_.end (), current);

		switch (PlayMode_)
		{
		case PlayMode::Sequential:
			if (pos == CurrentQueue_.end ())
				return CurrentQueue_.value (0);
			else if (++pos != CurrentQueue_.end ())
				return *pos;
			else
				return {};
		case PlayMode::Shuffle:
			return GetRandomBy<int> (pos,
					[this] (const Phonon::MediaSource& source)
						{ return CurrentQueue_.indexOf (source); });
		case PlayMode::ShuffleAlbums:
			return GetRandomBy<QString> (pos,
					[this] (const Phonon::MediaSource& source)
						{ return GetMediaInfo (source).Album_; });
		case PlayMode::ShuffleArtists:
			return GetRandomBy<QString> (pos,
					[this] (const Phonon::MediaSource& source)
						{ return GetMediaInfo (source).Artist_; });
		case PlayMode::RepeatTrack:
			return current;
		case PlayMode::RepeatAlbum:
		{
			if (pos == CurrentQueue_.end ())
				return CurrentQueue_.value (0);

			const auto& curAlbum = GetMediaInfo (*pos).Album_;
			++pos;
			if (pos == CurrentQueue_.end () ||
					GetMediaInfo (*pos).Album_ != curAlbum)
			{
				do
					--pos;
				while (pos >= CurrentQueue_.begin () &&
						GetMediaInfo (*pos).Album_ == curAlbum);
				++pos;
			}
			return *pos;
		}
		case PlayMode::RepeatWhole:
			if (pos == CurrentQueue_.end () || ++pos == CurrentQueue_.end ())
				pos = CurrentQueue_.begin ();
			return *pos;
		}

		return {};
	}

	void Player::play (const QModelIndex& index)
	{
		if (CurrentStation_)
		{
			auto item = PlaylistModel_->itemFromIndex (index);
			if (item == RadioItem_)
				return;
			else
				UnsetRadio ();
		}

		if (index.data (Role::IsAlbum).toBool ())
		{
			play (index.child (0, 0));
			return;
		}

		if (!index.isValid ())
		{
			qWarning () << Q_FUNC_INFO
					<< "invalid index"
					<< index;
			return;
		}

		Source_->stop ();
		const auto& source = index.data (Role::Source).value<Phonon::MediaSource> ();
		Source_->setCurrentSource (source);
		Source_->play ();
	}

	void Player::previousTrack ()
	{
		const auto& current = Source_->currentSource ();

		Phonon::MediaSource next;
		if (PlayMode_ == PlayMode::Shuffle)
		{
			next = GetNextSource (current);
			if (next.type () == Phonon::MediaSource::Empty)
				return;
		}
		else
		{
			const auto pos = std::find (CurrentQueue_.begin (), CurrentQueue_.end (), current);
			if (pos == CurrentQueue_.begin ())
				return;

			next = pos == CurrentQueue_.end () ? CurrentQueue_.value (0) : *(pos - 1);
		}

		Source_->stop ();
		Source_->setCurrentSource (next);
		Source_->play ();
	}

	void Player::nextTrack ()
	{
		if (CurrentStation_)
		{
			Source_->clear ();
			qApp->processEvents ();
			CurrentStation_->RequestNewStream ();
			return;
		}

		const auto& current = Source_->currentSource ();
		const auto& next = GetNextSource (current);
		if (next.type () == Phonon::MediaSource::Empty)
			return;

		Source_->stop ();
		Source_->setCurrentSource (next);
		Source_->play ();
	}

	void Player::togglePause ()
	{
		if (Source_->state () == Phonon::PlayingState)
			Source_->pause ();
		else
		{
			const auto& current = Source_->currentSource ();
			const bool invalidSource = current.type () == Phonon::MediaSource::Invalid ||
				current.type () == Phonon::MediaSource::Empty;
			if (invalidSource)
				Source_->setCurrentSource (CurrentQueue_.value (0));
			Source_->play ();
		}
	}

	void Player::setPause ()
	{
		Source_->pause ();
	}

	void Player::stop ()
	{
		Source_->stop ();
		emit songChanged (MediaInfo ());

		if (CurrentStation_)
			UnsetRadio ();
	}

	void Player::clear ()
	{
		UnsetRadio ();

		PlaylistModel_->clear ();
		Items_.clear ();
		AlbumRoots_.clear ();
		CurrentQueue_.clear ();
		Url2Info_.clear ();
		Source_->clearQueue ();

		XmlSettingsManager::Instance ().setProperty ("LastSong", QString ());

		Core::Instance ().GetPlaylistManager ()->
				GetStaticManager ()->SetOnLoadPlaylist (CurrentQueue_);

		if (Source_->state () != Phonon::PlayingState)
			Source_->setCurrentSource ({});
	}

	void Player::shufflePlaylist ()
	{
		SetPlayMode (PlayMode::Sequential);

		auto queue = GetQueue ();
		std::random_shuffle (queue.begin (), queue.end ());
		ReplaceQueue (queue, false);
	}

	void Player::handleSorted ()
	{
		auto watcher = dynamic_cast<QFutureWatcher<QList<QPair<Phonon::MediaSource, MediaInfo>>>*> (sender ());
		continueAfterSorted (watcher->result ());
		emit playerAvailable (true);
	}

	void Player::continueAfterSorted (const QList<QPair<Phonon::MediaSource, MediaInfo>>& sources)
	{
		CurrentQueue_.clear ();

		QMetaObject::invokeMethod (PlaylistModel_, "modelAboutToBeReset");
		PlaylistModel_->blockSignals (true);

		QString prevAlbumRoot;

		Q_FOREACH (const auto& sourcePair, sources)
		{
			const auto& source = sourcePair.first;
			CurrentQueue_ << source;

			auto item = new QStandardItem ();
			item->setEditable (false);
			item->setData (QVariant::fromValue (source), Role::Source);
			switch (source.type ())
			{
			case Phonon::MediaSource::Stream:
				item->setText (tr ("Stream"));
				PlaylistModel_->appendRow (item);
				break;
			case Phonon::MediaSource::Url:
			{
				const auto& url = source.url ();

				auto info = Core::Instance ().TryURLResolve (url);
				if (!info && Url2Info_.contains (url))
					info = Url2Info_ [url];

				if (info)
					FillItem (item, *info);
				else
					item->setText (url.toString ());

				PlaylistModel_->appendRow (item);
				break;
			}
			case Phonon::MediaSource::LocalFile:
			{
				const auto& info = sourcePair.second;

				const auto& albumID = info.Album_;
				FillItem (item, info);
				if (albumID != prevAlbumRoot ||
						AlbumRoots_ [albumID].isEmpty ())
				{
					PlaylistModel_->appendRow (item);

					if (!info.Album_.simplified ().isEmpty ())
						AlbumRoots_ [albumID] << item;
				}
				else if (AlbumRoots_ [albumID].last ()->data (Role::IsAlbum).toBool ())
				{
					IncAlbumLength (AlbumRoots_ [albumID].last (), info.Length_);
					AlbumRoots_ [albumID].last ()->appendRow (item);
				}
				else
				{
					auto albumItem = MakeAlbumItem (info);

					const int row = AlbumRoots_ [albumID].last ()->row ();
					const auto& existing = PlaylistModel_->takeRow (row);
					albumItem->appendRow (existing);
					albumItem->appendRow (item);
					PlaylistModel_->insertRow (row, albumItem);

					const auto& existingInfo = existing.at (0)->data (Role::Info).value<MediaInfo> ();
					albumItem->setData (existingInfo.Length_, Role::AlbumLength);
					IncAlbumLength (albumItem, info.Length_);

					emit insertedAlbum (albumItem->index ());

					AlbumRoots_ [albumID].last () = albumItem;
				}
				prevAlbumRoot = albumID;
				break;
			}
			default:
				item->setText ("unknown");
				PlaylistModel_->appendRow (item);
				break;
			}

			if (item)
				Items_ [source] = item;
		}

		PlaylistModel_->blockSignals (false);

		QMetaObject::invokeMethod (PlaylistModel_, "modelReset");

		Core::Instance ().GetPlaylistManager ()->
				GetStaticManager ()->SetOnLoadPlaylist (CurrentQueue_);

		if (Source_->state () == Phonon::StoppedState)
		{
			const auto& song = XmlSettingsManager::Instance ().property ("LastSong").toString ();
			if (!song.isEmpty ())
			{
				const auto pos = std::find_if (CurrentQueue_.begin (), CurrentQueue_.end (),
						[&song] (decltype (CurrentQueue_.front ()) item) { return song == item.fileName (); });
				if (pos != CurrentQueue_.end ())
					Source_->setCurrentSource (*pos);
			}
		}

		const auto& currentSource = Source_->currentSource ();
		if (Items_.contains (currentSource))
			Items_ [currentSource]->setData (true, Role::IsCurrent);
	}

	void Player::restorePlaylist ()
	{
		auto staticMgr = Core::Instance ().GetPlaylistManager ()->GetStaticManager ();
		Enqueue (staticMgr->GetOnLoadPlaylist ());
	}

	void Player::handleStationError (const QString& error)
	{
		const auto& e = Util::MakeNotification ("LMP",
				tr ("Radio station error: %1.")
					.arg (error),
				PCritical_);
		Core::Instance ().SendEntity (e);
	}

	void Player::handleRadioStream (const QUrl& url, const Media::AudioInfo& info)
	{
		Url2Info_ [url] = info;
		Source_->enqueue (url);

		qDebug () << Q_FUNC_INFO << Source_->state ();
		if (Source_->state () == Phonon::StoppedState)
			Source_->play ();
	}

	void Player::handleGotRadioPlaylist (const QString& name, const QString& format)
	{
		QMetaObject::invokeMethod (this,
				"postPlaylistCleanup",
				Qt::QueuedConnection,
				Q_ARG (QString, name));

		auto parser = MakePlaylistParser (format);
		if (!parser)
		{
			qWarning () << Q_FUNC_INFO
					<< "unable to find parser for format"
					<< format;
			return;
		}

		const auto& list = parser (name);
		Enqueue (list, false);
	}

	void Player::postPlaylistCleanup (const QString& filename)
	{
		UnsetRadio ();
		QFile::remove (filename);
	}

	void Player::handleUpdateSourceQueue ()
	{
		if (CurrentStation_)
		{
			Url2Info_.remove (Source_->currentSource ().url ());
			CurrentStation_->RequestNewStream ();
			return;
		}

		const auto& current = Source_->currentSource ();
		const auto& path = current.fileName ();
		if (!path.isEmpty ())
			QMetaObject::invokeMethod (Core::Instance ().GetLocalCollection (),
					"recordPlayedTrack",
					Qt::QueuedConnection,
					Q_ARG (QString, path));

		if (HandleCurrentStop (current))
			return;

		const auto& next = GetNextSource (current);
		if (next.type () != Phonon::MediaSource::Empty)
			Source_->enqueue (next);
	}

	void Player::handlePlaybackFinished ()
	{
		if (!Source_->queue ().isEmpty ())
		{
			auto queue = Source_->queue ();
			Source_->setCurrentSource (Source_->queue ().takeFirst ());
			Source_->play ();
			Source_->setQueue (queue);
		}
		else
		{
			emit songChanged (MediaInfo ());
			Source_->setCurrentSource ({});
		}
	}

	void Player::handleStateChanged (Phonon::State state)
	{
		qDebug () << Q_FUNC_INFO << state;
		if (state == Phonon::ErrorState)
			qDebug () << Source_->errorType () << Source_->errorString ();
	}

	void Player::handleCurrentSourceChanged (const Phonon::MediaSource& source)
	{
		XmlSettingsManager::Instance ().setProperty ("LastSong", source.fileName ());

		QStandardItem *curItem = 0;
		if (CurrentStation_)
			curItem = RadioItem_;
		else if (Items_.contains (source))
			curItem = Items_ [source];

		if (curItem)
			curItem->setData (true, Role::IsCurrent);

		if (Url2Info_.contains (source.url ()))
		{
			const auto& info = Url2Info_ [source.url ()];
			emit songChanged (info);
		}
		else if (curItem)
			emit songChanged (curItem->data (Role::Info).value<MediaInfo> ());
		else
			emit songChanged (MediaInfo ());

		if (curItem)
			emit indexChanged (PlaylistModel_->indexFromItem (curItem));

		Q_FOREACH (auto item, Items_.values ())
		{
			if (item == curItem)
				continue;
			if (item->data (Role::IsCurrent).toBool ())
			{
				item->setData (false, Role::IsCurrent);
				break;
			}
		}
	}

	void Player::handleMetadata ()
	{
		const auto& source = Source_->currentSource ();
		const auto& isUrl = source.type () == Phonon::MediaSource::Stream ||
				(source.type () == Phonon::MediaSource::Url && source.url ().scheme () != "file");
		if (!isUrl ||
				CurrentStation_ ||
				!Items_.contains (source))
			return;

		auto curItem = Items_ [source];

		const auto& info = GetPhononMediaInfo ();
		if (info.Album_ == LastPhononMediaInfo_.Album_ &&
				info.Artist_ == LastPhononMediaInfo_.Artist_ &&
				info.Title_ == LastPhononMediaInfo_.Title_)
			return;

		LastPhononMediaInfo_ = info;

		FillItem (curItem, info);
		emit songChanged (info);
	}

	void Player::refillPlaylist ()
	{
		ReplaceQueue (GetQueue (), false);
	}

	void Player::setTransitionTime ()
	{
		const int time = XmlSettingsManager::Instance ()
				.property ("TransitionTime").toInt ();
		Source_->setTransitionTime (time);
	}
}
}
