#pragma once
#ifndef _MAJEST_LYRIC_DECODER_
#define _MAJEST_LYRIC_DECODER_
#include <decoders/DecoderTypes.h>
#include <core/Album.h>
#include <util/StringUtil.h>

namespace MajestLyrics
{
	static inline std::string WideToUTF8(const std::wstring& s)
	{
		if (s.empty()) return "";
		int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
		if (len <= 0) return "";
		std::string result(len - 1, '\0');
		WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, &result[0], len, nullptr, nullptr);
		return result;
	}

	// Provider chain ordered by reliability and data quality.
	// Musixmatch is skipped automatically when no API key is configured.
	// On full failure, inserts an empty sentinel to prevent repeated requests.
	static void TryDecode(const std::string& artist, const std::string& song, Album& out_album)
	{
		if (LrcLibDecoder().DecodeLyrics(artist, song, out_album))     return; // free, synced+plain
		if (LyricsOvhDecoder().DecodeLyrics(artist, song, out_album))  return; // free, plain
		if (MusixmatchDecoder().DecodeLyrics(artist, song, out_album)) return; // key required, plain
		if (ChartLyricsDecoder().DecodeLyrics(artist, song, out_album)) return; // free, XML, plain
		if (LyricaDecoder().DecodeLyrics(artist, song, out_album))     return; // aggregator, experimental
		out_album.name = UTF8ToWide(artist);
		out_album.songs[SongKey(UTF8ToWide(artist), UTF8ToWide(song))] = L"";
	}
}

#endif
