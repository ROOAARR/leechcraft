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

#include "decompressor.h"
#include <stdexcept>
#include <cstring>
#include <QByteArray>
#include <QtDebug>
#include "mobiparser.h"
#include "util.h"

namespace LeechCraft
{
namespace Monocle
{
namespace Dik
{
	Decompressor::~Decompressor ()
	{
	}

	class NullDecompressor : public Decompressor
	{
	public:
		QByteArray operator() (const QByteArray& data)
		{
			return data;
		}
	};

	namespace
	{
		const static uchar TokenCode [256] =
		{
			0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
			3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
			3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
			3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
			2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		};
	}

	class RLEDecompressor : public Decompressor
	{
	public:
		QByteArray operator() (const QByteArray&);
	};

	QByteArray RLEDecompressor::operator() (const QByteArray& data)
	{
		QByteArray result;

		const int maxIndex = data.size () - 1;

		for (int i = 0; i < data.size (); )
		{
			bool end = false;
			const uchar token = data.at (i++);
			switch (TokenCode [token])
			{
			case 0:
				result.append (token);
				break;
			case 1:
				if (i + token > maxIndex)
				{
					end = true;
					break;
				}
				result.append (data.mid (i, token));
				i += token;
				break;
			case 2:
				result.append (' ');
				result.append (token ^ 0x80);
				break;
			case 3:
			{
				if (i + 1 > maxIndex)
				{
					end = true;
					break;
				}

				uint16_t n = token;
				n <<= 8;
				n += static_cast<uchar> (data.at (i++));

				const uint16_t copyLength = (n & 7) + 3;
				const uint16_t shift = (n & 0x3fff) / 8;
				const auto shifted = result.size () - shift;
				if (shifted > result.size () - 1)
				{
					end = true;
					break;
				}

				for (uint16_t j = 0; j < copyLength; ++j)
					result.append (result.at (shifted + j));
			}
			}

			if (end)
				break;
		}

		return result;
	}

	namespace
	{
		class ToBits
		{
			const QByteArray Data_;

			quint32 Pos_ = 0;
			const quint32 Len_;
		public:
			ToBits (const QByteArray& ba)
			: Data_ (ba + "\0\0\0\0")
			, Len_ (Data_.size () * 8)
			{
			}

			quint32 operator() ()
			{
				quint32 g = 0;
				quint64 r = 0;
				while (g < 32)
				{
					r = (r << 8) | static_cast<quint8> (Data_ [(Pos_ + g) >> 3]);
					g = g + 8 - ((Pos_ + g) & 7);
				}

				return r >> (g - 32);
			}

			bool operator+= (quint32 n)
			{
				Pos_ += n;
				return Pos_ <= Len_;
			}

			qint64 Avail () const
			{
				return Len_ - Pos_;
			}
		};
	}

	class HuffDecompressor : public Decompressor
	{
		const MobiParser * const P_;

		QList<QByteArray> Dicts_;
		quint32 EntryBits_;

		quint32 Dict1_ [256];
		quint32 Dict2_ [64];

		QByteArray Buf_;
	public:
		HuffDecompressor (const MobiParser*);
		QByteArray operator() (const QByteArray&);
	private:
		void Unpack (ToBits, int);
	};

	HuffDecompressor::HuffDecompressor (const MobiParser *parser)
	: P_ (parser)
	{
		const auto& header = parser->GetRecord (0);

		auto read32 = [&header] (int offset) { return Read32 (header, offset); };

		const quint32 huffOffset = read32 (0x70);
		const quint32 huffNum = read32 (0x74);

		const auto& huff1 = parser->GetRecord (huffOffset);
		if (huff1.isEmpty ())
			throw std::runtime_error ("cannot get HUFF record");

		for (uint i = 1; i < huffNum; ++i)
		{
			const auto& nextH = parser->GetRecord (huffOffset + i);
			if (nextH.isEmpty ())
				throw std::runtime_error ("cannot get HUFF record");

			Dicts_ << nextH;
		}

		if (!huff1.startsWith ("HUFF") ||
				!Dicts_.value (0).startsWith ("CDIC"))
			throw std::runtime_error ("invalid HUFF records format");

		EntryBits_ = Read32 (Dicts_.value (0), 0xc);

		const quint32 off1 = Read32 (huff1, 0x10);
		const quint32 off2 = Read32 (huff1, 0x14);

		std::memcpy (Dict1_, huff1.data () + off1, 256 * 4);
		std::memcpy (Dict2_, huff1.data () + off2, 64 * 4);
	}

	void HuffDecompressor::Unpack (ToBits reader, int depth)
	{
		if (depth > 64)
			throw std::runtime_error ("recursion depth exceeded");

		while (reader.Avail () > 0)
		{
			const auto dw = reader ();
			const auto v = Dict1_ [dw >> 24];
			quint8 codelen = v & 0x1f;

			if (!codelen)
				throw std::runtime_error ("invalid huff code");

			quint32 code = dw >> (32 - codelen);
			quint32 r = v >> 8;

			if (!(v & 0x80))
			{
				while (code < Dict2_ [2 * codelen - 2])
				{
					++codelen;
					code = dw >> (32 - codelen);
				}
				r = Dict2_ [codelen * 2 - 1];
			}

			r -= code;

			if (!codelen)
				throw std::runtime_error ("invalid huff code");

			if (!(reader += codelen))
				return;

			const quint32 nDict = r >> EntryBits_;
			const auto& dict = Dicts_ [nDict];

			const quint32 off1 = 16 + (r - (nDict << EntryBits_)) * 2;
			const quint32 off2 = 16 + static_cast<uchar> (dict [off1]) * 256 + static_cast<uchar> (dict [off1 + 1]);
			const quint32 blen = static_cast<uchar> (dict [off2]) * 256 + static_cast<uchar> (dict [off2 + 1]);

			const auto& app = dict.mid (off2 + 2, (blen & 0x7fff));
			if (blen & 0x8000)
				Buf_ += app;
			else
				Unpack ({ app }, depth + 1);
		}
	}

	QByteArray HuffDecompressor::operator() (const QByteArray& data)
	{
		Unpack ({ data }, 0);
		QByteArray result;
		std::swap (result, Buf_);
		return result;
	}

	Decompressor_ptr Decompressor::Create (Type type, const MobiParser *p)
	{
		switch (type)
		{
		case Type::None:
			return Decompressor_ptr (new NullDecompressor);
		case Type::RLE:
			return Decompressor_ptr (new RLEDecompressor);
		case Type::Huff:
			return Decompressor_ptr (new HuffDecompressor (p));
		}

		qWarning () << Q_FUNC_INFO
				<< "unknown decompressor type";
		return {};
	}
}
}
}
