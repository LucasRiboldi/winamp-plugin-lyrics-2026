#pragma once
#ifndef _LYRICS_OVH_DECODER_
#define _LYRICS_OVH_DECODER_
#include <core/Decoder.h>
#include <net/HttpClient.h>
#include <util/StringUtil.h>
#include <util/JsonUtil.h>

// Fetches plain lyrics from api.lyrics.ovh — free, no API key required.
// GET https://api.lyrics.ovh/v1/{artist}/{title}
// Response: {"lyrics": "..."}  |  {"error": "No lyrics found"} on miss
class LyricsOvhDecoder : public Decoder
{
public:
	bool DecodeLyrics(const std::string& artist, const std::string& song, MajestLyrics::Album& out_album) override
	{
		const std::string url =
			"https://api.lyrics.ovh/v1/" + UrlEncode(artist) + "/" + UrlEncode(song);

		std::string data = HttpClient::Get(url);
		if (data == "failed" || data == "failed_tls") return false;

		std::string lyrics = ExtractJsonString(data, "lyrics");
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
