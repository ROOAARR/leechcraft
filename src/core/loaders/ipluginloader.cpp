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

#include "ipluginloader.h"
#if defined __GNUC__
#include <cxxabi.h>
#endif
#include <QLibrary>

namespace LeechCraft
{
namespace Loaders
{
	namespace
	{
		QString TryDemangle (const QString& errorStr)
		{
#if defined __GNUC__
			const QString marker ("undefined symbol: ");
			const auto pos = errorStr.indexOf (marker);
			if (pos == -1)
				return QString ();

			auto mangled = errorStr.mid (pos + marker.size ());
			const auto endPos = mangled.indexOf (')');
			if (endPos >= 0)
				mangled = mangled.left (endPos);

			int status = 0;
			QString result;
			if (auto rawStr = abi::__cxa_demangle (mangled.toLatin1 ().constData (), 0, 0, &status))
			{
				result = QString::fromLatin1 (rawStr);
				free (rawStr);
			}
			return result;
#else
			return QString ();
#endif
		}
	}

	qint64 GetLibAPILevel (const QString& file)
	{
		if (file.isEmpty ())
			return static_cast<quint64> (-1);

		QLibrary library (file);
		if (!library.load ())
			return static_cast<quint64> (-1);

		typedef quint64 (*APIVersion_t) ();
		auto getter = reinterpret_cast<APIVersion_t> (library.resolve ("GetAPILevels"));
		return getter ? getter () : static_cast<quint64> (-1);
	}
}
}