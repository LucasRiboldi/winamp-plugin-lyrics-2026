#pragma once
#ifndef _LYRICA_DECODER_
#define _LYRICA_DECODER_
#include <core/Decoder.h>
#include <net/HttpClient.h>
#include <util/StringUtil.h>
#include <util/JsonUtil.h>

// Fetches lyrics from the Lyrica aggregator REST API — no auth required.
// Aggregates from: Genius, LRCLIB, SimpMusic, YouTube Music, Lyrics.ovh, ChartLyrics.
// Rate limit: 15 req/min per IP.
//
// Hosted instance: https://github.com/Wilooper/Lyrica
// This is a community-run server on Render.com — treat as best-effort fallback.
// Response may contain the lyrics in "lyrics", "plain_lyrics", or "lyrics_text".
class LyricaDecoder : public Decoder
{
public:
	bool DecodeLyrics(const std::string& artist, const std::string& song, MajestLyrics::Album& out_album) override
	{
		const std::string url =
			"https://test-0k.onrender.com/lyrics/"
			"?artist=" + UrlEncode(artist) +
			"&song="   + UrlEncode(song);

		std::string data = HttpClient::Get(url);
		if (data == "failed" || data == "failed_tls") return false;

		// The Lyrica API may use any of these field names depending on the source.
		std::string lyrics = ExtractJsonString(data, "lyrics");
		if (lyrics.empty()) lyrics = ExtractJsonString(data, "plain_lyrics");
		if (lyrics.empty()) lyrics = ExtractJsonString(data, "lyrics_text");
		if (lyrics.empty()) return false;

		out_album.name = UTF8ToWide(artist);
		std::wstring wideLyrics = UTF8ToWide(lyrics);
		StripCR(wideLyrics);
		out_album.songs[SongKey(UTF8ToWide(artist), UTF8ToWide(song))] = std::move(wideLyrics);
		return true;
	}

private:
	void Parse(const std::string&, MajestLyrics::Album&) override {}
};

#endif
