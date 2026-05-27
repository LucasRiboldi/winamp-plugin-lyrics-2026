#pragma once
#ifndef _CHARTLYRICS_DECODER_
#define _CHARTLYRICS_DECODER_
#include <core/Decoder.h>
#include <net/HttpClient.h>
#include <util/StringUtil.h>
#include <unordered_map>

static inline std::string ExtractXmlTag(const std::string& xml, const std::string& tag)
{
	const std::string open  = "<" + tag + ">";
	const std::string close = "</" + tag + ">";
	size_t start = xml.find(open);
	if (start == std::string::npos) return "";
	start += open.size();
	size_t end = xml.find(close, start);
	return end == std::string::npos ? "" : xml.substr(start, end - start);
}

// Fetches lyrics via ChartLyrics API (fallback source).
class ChartLyricsDecoder : public Decoder
{
public:
	bool DecodeLyrics(const std::string& artist, const std::string& song, MajestLyrics::Album& out_album) override
	{
		const std::string url =
			"https://sridurgayadav-chart-lyrics-v1.p.rapidapi.com/apiv1.asmx/SearchLyricDirect"
			"?artist=" + UrlEncode(artist) +
			"&song="   + UrlEncode(song);

		const std::unordered_map<std::string, std::string> hdrs = {
			{ "x-rapidapi-key",  "2df0f14d1fmsh669835e13e12203p14d1f4jsn19f39db18d4e" },
			{ "x-rapidapi-host", "sridurgayadav-chart-lyrics-v1.p.rapidapi.com" },
			{ "Content-Type",    "application/json" }
		};

		std::string data = HttpClient::Get(url, false, hdrs);
		if (data == "failed") return false;

		std::string lyrics = ExtractXmlTag(data, "Lyric");
		if (lyrics.empty() || lyrics == "Not found") return false;

		out_album.name = UTF8ToWide(artist);
		out_album.songs[ToLower(UTF8ToWide(song))] = UTF8ToWide(lyrics);
		return true;
	}

private:
	void Parse(const std::string&, MajestLyrics::Album&) override {}
};

#endif
