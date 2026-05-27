#pragma once
#ifndef _LRCLIB_DECODER_
#define _LRCLIB_DECODER_
#include <core/Decoder.h>
#include <net/HttpClient.h>
#include <util/StringUtil.h>

static inline std::string ExtractJsonString(const std::string& json, const std::string& key)
{
	std::string search = "\"" + key + "\":\"";
	size_t pos = json.find(search);
	if (pos == std::string::npos) return "";
	pos += search.size();

	const size_t MAX_LYRICS_BYTES = 65536;
	std::string result;
	result.reserve(1024);
	while (pos < json.size() && json[pos] != '"' && result.size() < MAX_LYRICS_BYTES)
	{
		if (json[pos] == '\\' && pos + 1 < json.size())
		{
			switch (json[++pos])
			{
				case 'n':  result += '\n'; break;
				case 'r':  result += '\r'; break;
				case '"':  result += '"';  break;
				case '\\': result += '\\'; break;
				case 't':  result += '\t'; break;
				case 'u':  pos += 4;       break; // skip \uXXXX sequences
				default:                   break;
			}
		}
		else result += json[pos];
		++pos;
	}
	return result;
}

// Fetches plain lyrics from lrclib.net — free, no API key required.
class LrcLibDecoder : public Decoder
{
public:
	bool DecodeLyrics(const std::string& artist, const std::string& song, MajestLyrics::Album& out_album) override
	{
		const std::string url =
			"https://lrclib.net/api/get"
			"?artist_name=" + UrlEncode(artist) +
			"&track_name="  + UrlEncode(song);

		std::string data = HttpClient::Get(url);
		if (data == "failed" || data == "failed_tls") return false;

		std::string lyrics = ExtractJsonString(data, "plainLyrics");
		if (lyrics.empty()) return false;

		out_album.name = UTF8ToWide(artist);
		out_album.songs[ToLower(UTF8ToWide(song))] = UTF8ToWide(lyrics);
		return true;
	}

private:
	void Parse(const std::string&, MajestLyrics::Album&) override {}
};

#endif
