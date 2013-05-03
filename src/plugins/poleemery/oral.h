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

#include <stdexcept>
#include <type_traits>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/fold.hpp>
#include <boost/fusion/include/filter_if.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/transform.hpp>
#include <boost/fusion/include/zip.hpp>
#include <QStringList>
#include <QDateTime>
#include <QPair>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>
#include "oraltypes.h"

namespace LeechCraft
{
namespace Poleemery
{
namespace oral
{
	class QueryException : public std::runtime_error
	{
		const QSqlQuery Query_;
	public:
		QueryException (const std::string& str, const QSqlQuery& q)
		: std::runtime_error (str)
		, Query_ (q)
		{
		}

		virtual ~QueryException () throw ()
		{
		}

		const QSqlQuery& GetQuery () const
		{
			return Query_;
		}
	};

	template<typename T>
	struct PKey
	{
		typedef T value_type;

		T Val_;

		PKey& operator= (const value_type& val)
		{
			Val_ = val;
			return *this;
		}

		operator value_type () const
		{
			return Val_;
		}
	};

	namespace detail
	{
		template<typename T>
		struct IsPKey : std::false_type {};

		template<typename U>
		struct IsPKey<PKey<U>> : std::true_type {};
	}

	template<typename Seq, int Idx>
	struct References
	{
		typedef typename std::decay<typename boost::fusion::result_of::at_c<Seq, Idx>::type>::type member_type;
		static_assert (detail::IsPKey<member_type>::value, "References<> element must refer to a PKey<> element");

		typedef typename member_type::value_type value_type;
		value_type Val_;

		References& operator= (const value_type& val)
		{
			Val_ = val;
			return *this;
		}

		operator value_type () const
		{
			return Val_;
		}
	};

	namespace detail
	{
		template<typename T1, typename T2, template<typename U> class Container, typename F>
		auto ZipWith (const Container<T1>& c1, const Container<T2>& c2, F f) -> Container<decltype (f (T1 (), T2 ()))>
		{
			Container<decltype (f (T1 (), T2 ()))> result;
			for (auto i1 = std::begin (c1), e1 = std::end (c1),
						i2 = std::begin (c2), e2 = std::end (c2);
					i1 != e1 && i2 != e2; ++i1, ++i2)
				result.push_back (f (*i1, *i2));
			return result;
		}

		template<typename T, template<typename U> class Container, typename F>
		auto Map (const Container<T>& c, F f) -> Container<decltype (f (T ()))>
		{
			Container<decltype (f (T ()))> result;
			for (auto t : c)
				result.push_back (f (t));
			return result;
		}

		template<typename Seq, int Idx>
		struct GetFieldName
		{
			static QString value () { return boost::fusion::extension::struct_member_name<Seq, Idx>::call (); }
		};

		template<typename Seq, int Idx>
		struct GetBoundName
		{
			static QString value () { return ':' + Seq::ClassName () + "_" + GetFieldName<Seq, Idx>::value (); }
		};

		template<typename S, typename N>
		struct GetFieldsNames_
		{
			QStringList operator() () const
			{
				return QStringList { GetFieldName<S, N::value>::value () } + GetFieldsNames_<S, typename boost::mpl::next<N>::type> {} ();
			}
		};

		template<typename S>
		struct GetFieldsNames_<S, typename boost::fusion::result_of::size<S>::type>
		{
			QStringList operator() () const
			{
				return {};
			}
		};

		template<typename S>
		struct GetFieldsNames : GetFieldsNames_<S, boost::mpl::int_<0>>
		{
		};
	}

	template<typename T>
	struct Type2Name;

	template<>
	struct Type2Name<int>
	{
		QString operator() () const { return  "INTEGER"; }
	};

	template<>
	struct Type2Name<QString>
	{
		QString operator() () const { return "TEXT"; }
	};

	template<typename T>
	struct Type2Name<PKey<T>>
	{
		QString operator() () const { return Type2Name<T> () () + " PRIMARY KEY"; }
	};

	template<>
	struct Type2Name<PKey<int>>
	{
		QString operator() () const { return Type2Name<int> () () + " PRIMARY KEY AUTOINCREMENT"; }
	};

	template<typename Seq, int Idx>
	struct Type2Name<References<Seq, Idx>>
	{
		QString operator() () const
		{
			return Type2Name<typename References<Seq, Idx>::value_type> () () +
					" REFERENCES " + Seq::ClassName () + " (" + detail::GetFieldName<Seq, Idx>::value () + ")";
		}
	};

	namespace detail
	{
		struct Types
		{
			typedef QStringList result_type;

			template<typename T>
			QStringList operator() (const QStringList& init, const T&) const
			{
				return init + QStringList { Type2Name<T> () () };
			}
		};
	}

	template<typename T>
	struct ToVariant
	{
		QVariant operator() (const T& t) const
		{
			return t;
		}
	};

	template<typename T>
	struct ToVariant<PKey<T>>
	{
		QVariant operator() (const PKey<T>& t) const
		{
			return static_cast<typename PKey<T>::value_type> (t);
		}
	};

	template<typename Seq, int Idx>
	struct ToVariant<References<Seq, Idx>>
	{
		QVariant operator() (const References<Seq, Idx>& t) const
		{
			return static_cast<typename References<Seq, Idx>::value_type> (t);
		}
	};

	namespace detail
	{
		struct Inserter
		{
			typedef QStringList result_type;

			QSqlQuery& Q_;

			template<typename T>
			QStringList operator() (QStringList bounds, const T& t) const
			{
				Q_.bindValue (bounds.takeFirst (), ToVariant<T> {} (t));
				return bounds;
			}
		};
	}

	template<typename T>
	struct FromVariant
	{
		T operator() (const QVariant& var) const
		{
			return var.value<T> ();
		}
	};

	template<typename T>
	struct FromVariant<PKey<T>>
	{
		T operator() (const QVariant& var) const
		{
			return var.value<T> ();
		}
	};

	template<typename Seq, int Idx>
	struct FromVariant<References<Seq, Idx>>
	{
		typedef typename References<Seq, Idx>::value_type value_type;

		value_type operator() (const QVariant& var) const
		{
			return var.value<value_type> ();
		}
	};

	namespace detail
	{
		struct Selector
		{
			typedef int result_type;

			const QSqlQuery& Q_;

			template<typename T>
			int operator() (int index, T& t) const
			{
				t = FromVariant<T> {} (Q_.value (index));
				return index + 1;
			}
		};

		struct CachedFieldsData
		{
			const QString Table_;
			const QSqlDatabase& DB_;

			QList<QString> Fields_;
			QList<QString> BoundFields_;
		};

		template<typename T>
		std::function<void (T)> MakeInserter (CachedFieldsData data, const QSqlQuery& insertQuery)
		{
			return [data, insertQuery] (const T& t)
			{
				auto q = insertQuery;
				boost::fusion::fold (t, data.BoundFields_, Inserter { q });
				if (!q.exec ())
					throw QueryException ("insert query execution failed", q);
			};
		}

		template<typename T>
		QPair<QSqlQuery, std::function<void (T)>> AdaptInsert (CachedFieldsData data)
		{
			const auto& insert = "INSERT INTO " + data.Table_ +
					" (" + QStringList { data.Fields_ }.join (", ") + ") VALUES (" +
					QStringList { data.BoundFields_ }.join (", ") + ");";
			QSqlQuery insertQuery (data.DB_);
			insertQuery.prepare (insert);
			return { insertQuery, MakeInserter<T> (data, insertQuery) };
		}

		template<typename T>
		struct Lazy
		{
			typedef T type;
		};

		template<typename Seq, typename MemberIdx = boost::mpl::int_<0>>
		struct FindPKey
		{
			static_assert ((boost::fusion::result_of::size<Seq>::value) != (MemberIdx::value),
					"Primary key not found");

			typedef typename boost::fusion::result_of::at<Seq, MemberIdx>::type item_type;
			typedef typename std::conditional<
						IsPKey<typename std::decay<item_type>::type>::value,
						Lazy<MemberIdx>,
						Lazy<FindPKey<Seq, typename boost::mpl::next<MemberIdx>>>
					>::type::type result_type;
		};

		template<typename T>
		QPair<QSqlQuery, std::function<void (T)>> AdaptUpdate (CachedFieldsData data)
		{
			const auto index = FindPKey<T>::result_type::value;

			data.Fields_.removeAt (index);
			data.BoundFields_.removeAt (index);

			const auto& statements = ZipWith (data.Fields_, data.BoundFields_,
					[] (const QString& s1, const QString& s2) -> QString
						{ return s1 + " = " + s2; });

			const auto& fieldName = GetFieldName<T, index>::value ();
			const auto& boundName = GetBoundName<T, index>::value ();

			const auto& update = "UPDATE " + data.Table_ +
					" SET " + QStringList { statements }.join (", ") +
					" WHERE " + fieldName + " = " + boundName + ";";

			QSqlQuery updateQuery (data.DB_);
			updateQuery.prepare (update);

			return { updateQuery, MakeInserter<T> (data, updateQuery) };
		}

		template<typename T>
		QList<T> PerformSelect (QSqlQuery q)
		{
			if (!q.exec ())
				throw QueryException ("fetch query execution failed", q);

			QList<T> result;
			while (q.next ())
			{
				T t;
				boost::fusion::fold (t, 0, Selector { q });
				result << t;
			}
			q.finish ();
			return result;
		}

		template<typename T>
		QPair<QSqlQuery, std::function<QList<T> ()>> AdaptSelectAll (const CachedFieldsData& data)
		{
			const auto& selectAll = "SELECT " + QStringList { data.Fields_ }.join (", ") + " FROM " + data.Table_ + ";";
			QSqlQuery selectQuery (data.DB_);
			selectQuery.prepare (selectAll);
			auto selector = [selectQuery] () { return PerformSelect<T> (selectQuery); };
			return { selectQuery, selector };
		}

		template<typename OrigSeq, typename OrigIdx, typename RefSeq, typename MemberIdx>
		struct FieldInfo
		{
		};

		template<typename To, typename OrigSeq, typename OrigIdx, typename T>
		struct FieldAppender
		{
			typedef To value_type;
		};

		template<typename To, typename OrigSeq, typename OrigIdx, typename RefSeq, int RefIdx>
		struct FieldAppender<To, OrigSeq, OrigIdx, References<RefSeq, RefIdx>>
		{
			typedef typename boost::fusion::result_of::as_vector<
					typename boost::fusion::result_of::push_back<
						To,
						FieldInfo<OrigSeq, OrigIdx, RefSeq, boost::mpl::int_<RefIdx>>
					>::type
				>::type value_type;
		};

		template<typename Seq, typename MemberIdx>
		struct CollectRefs_
		{
			typedef typename FieldAppender<
					typename CollectRefs_<Seq, typename boost::mpl::next<MemberIdx>::type>::type_list,
					Seq,
					MemberIdx,
					typename std::decay<typename boost::fusion::result_of::at<Seq, MemberIdx>::type>::type
				>::value_type type_list;
		};

		template<typename Seq>
		struct CollectRefs_<Seq, typename boost::fusion::result_of::size<Seq>::type>
		{
			typedef boost::fusion::vector<> type_list;
		};

		template<typename Seq>
		struct CollectRefs : CollectRefs_<Seq, boost::mpl::int_<0>>
		{
		};

		struct Ref2Select
		{
			typedef QStringList result_type;

			template<typename OrigSeq, typename OrigIdx, typename RefSeq, typename RefIdx>
			QStringList operator() (const QStringList& init, const FieldInfo<OrigSeq, OrigIdx, RefSeq, RefIdx>&) const
			{
				const auto& thisQualified = OrigSeq::ClassName () + "." + GetFieldName<OrigSeq, OrigIdx::value>::value ();
				return init + QStringList { thisQualified + " = " + GetBoundName<RefSeq, RefIdx::value>::value () };
			}
		};

		template<typename T>
		struct ExtrObj;

		template<typename OrigSeq, typename OrigIdx, typename RefSeq, typename MemberIdx>
		struct ExtrObj<FieldInfo<OrigSeq, OrigIdx, RefSeq, MemberIdx>>
		{
			typedef RefSeq type;
		};

		struct SingleBind
		{
			QSqlQuery& Q_;

			template<typename ObjType, typename OrigSeq, typename OrigIdx, typename RefSeq, typename RefIdx>
			void operator() (const boost::fusion::vector2<ObjType, const FieldInfo<OrigSeq, OrigIdx, RefSeq, RefIdx>&>& pair) const
			{
				Q_.bindValue (GetBoundName<RefSeq, RefIdx::value>::value (),
						ToVariant<typename std::decay<typename boost::fusion::result_of::at<RefSeq, RefIdx>::type>::type> () (boost::fusion::at<RefIdx> (boost::fusion::at_c<0> (pair))));
			}
		};

		template<typename T, typename RefSeq>
		struct MakeBinder
		{
			typedef typename boost::mpl::transform<RefSeq, ExtrObj<boost::mpl::_1>> transform_view;
			typedef typename transform_view::type objects_view;
			typedef typename boost::fusion::result_of::as_vector<objects_view>::type objects_vector;

			QSqlQuery Q_;

			QList<T> operator() (const objects_vector& objs)
			{
				boost::fusion::for_each (boost::fusion::zip (objs, RefSeq {}), SingleBind { Q_ });
				return PerformSelect<T> (Q_);
			}
		};

		template<typename T, typename ObjInfo>
		typename std::enable_if<CollectRefs<T>::type_list::size::value >= 1>::type AdaptSelectRef (const CachedFieldsData& data, ObjInfo& info)
		{
			typedef typename CollectRefs<T>::type_list references_list;
			const auto& statements = boost::fusion::fold (references_list {}, QStringList {}, Ref2Select {});

			const auto& selectAll = "SELECT " + QStringList { data.Fields_ }.join (", ") +
					" FROM " + data.Table_ +
					(statements.isEmpty () ? "" : " WHERE ") + statements.join (" AND ") +
					";";
			QSqlQuery selectQuery (data.DB_);
			selectQuery.prepare (selectAll);

			info.SelectByFKeys_ = selectQuery;
			info.SelectByFKeysActor_ = MakeBinder<T, references_list> { selectQuery };
		}

		template<typename T, typename ObjInfo>
		typename std::enable_if<CollectRefs<T>::type_list::size::value <= 0>::type AdaptSelectRef (const CachedFieldsData&, ObjInfo&)
		{
		}

		template<typename T>
		QString AdaptCreateTable (const CachedFieldsData& data)
		{
			const QList<QString> types = boost::fusion::fold (T {}, QStringList {}, Types {});

			auto statements = ZipWith (types, data.Fields_,
					[] (const QString& type, const QString& field) -> QString { return field + " " + type; });
			return "CREATE TABLE " + data.Table_ +  " (" + QStringList { statements }.join (", ") + ");";
		}

		template<typename T, typename Enable = void>
		struct ObjectInfoFKeysHelper
		{
		};

		template<typename T>
		struct ObjectInfoFKeysHelper<T, typename std::enable_if<CollectRefs<T>::type_list::size::value >= 1, void>::type>
		{
			QSqlQuery SelectByFKeys_;
			std::function<QList<T> (typename MakeBinder<T, typename CollectRefs<T>::type_list>::objects_vector)> SelectByFKeysActor_;
		};
	}

	template<typename T>
	struct ObjectInfo : detail::ObjectInfoFKeysHelper<T>
	{
		QSqlQuery QuerySelectAll_;
		std::function<QList<T> ()> DoSelectAll_;

		QSqlQuery QueryInsertOne_;
		std::function<void (T)> DoInsert_;

		QSqlQuery QueryUpdate_;
		std::function<void (T)> DoUpdate_;

		QSqlQuery QueryDelete_;
		std::function<void (T)> DoDelete_;

		QString CreateTable_;

		ObjectInfo ()
		{
		}

		ObjectInfo (decltype (QuerySelectAll_) sel, decltype (DoSelectAll_) doSel,
				decltype (QueryInsertOne_) insert, decltype (DoInsert_) doIns,
				decltype (QueryUpdate_) update, decltype (DoUpdate_) doUpdate,
				decltype (CreateTable_) createTable)
		: QuerySelectAll_ (sel)
		, DoSelectAll_ (doSel)
		, QueryInsertOne_ (insert)
		, DoInsert_ (doIns)
		, QueryUpdate_ (update)
		, DoUpdate_ (doUpdate)
		, CreateTable_ (createTable)
		{
		}
	};

	template<typename T>
	ObjectInfo<T> Adapt (const QSqlDatabase& db)
	{
		const QList<QString> fields = detail::GetFieldsNames<T> {} ();
		const QList<QString> boundFields = detail::Map (fields, [] (const QString& str) -> QString { return ':' + str; });

		const auto& table = T::ClassName ();

		const detail::CachedFieldsData cachedData { table, db, fields, boundFields };
		const auto& selectPair = detail::AdaptSelectAll<T> (cachedData);
		const auto& insertPair = detail::AdaptInsert<T> (cachedData);
		const auto& updatePair = detail::AdaptUpdate<T> (cachedData);
		const auto& createTable = detail::AdaptCreateTable<T> (cachedData);

		ObjectInfo<T> info
		{
			selectPair.first, selectPair.second,
			insertPair.first, insertPair.second,
			updatePair.first, updatePair.second,
			createTable
		};

		detail::AdaptSelectRef<T> (cachedData, info);

		return info;
	}
}
}
}