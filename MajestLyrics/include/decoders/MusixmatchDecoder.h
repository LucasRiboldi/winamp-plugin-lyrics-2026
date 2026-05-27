#pragma once
#ifndef _MUSIXMATCH_DECODER_
#define _MUSIXMATCH_DECODER_

// Musixmatch unofficial API — uses the key embedded in the Android app.
// No personal API key required. Token is obtained anonymously (supported since 2024)
// and cached in {plugin_dir}\musixmatch_token.txt between sessions.
//
// Source reference: https://codeberg.org/ThetaDev/musixmatch-inofficial
// Signing: HMAC-SHA1(full_url + YYYYMMDD, secret) → base64+"\n" → URL-encoded.

#include <core/Decoder.h>
#include <net/HttpClient.h>
#include <util/StringUtil.h>
#include <util/JsonUtil.h>
#include <Windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <fstream>
#include <vector>
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")

namespace MxmApi
{
	static const char SECRET[]    = "mNdca@6W7TeEcFn6*3.s97sJ*yPMd";
	static const char APP_ID[]    = "android-player-v1.0";
	static const char BASE_URL[]  = "https://apic.musixmatch.com/ws/1.1/";
	static const char BUILD[]     = "2022090901";
	static const char MODEL[]     = "Google Pixel 6; Android 13";
	static const char UA[]        = "Dalvik/2.1.0 (Linux; U; Android 13; Pixel 6 Build/T3B2.230316.003)";
	static const char COOKIE[]    = "AWSELBCORS=0; AWSELB=0";

	// Computes HMAC-SHA1 of |message| with SECRET and returns base64-encoded bytes + "\n".
	static inline std::string Sign(const std::string& message)
	{
		BCRYPT_ALG_HANDLE  hAlg  = NULL;
		BCRYPT_HASH_HANDLE hHash = NULL;

		BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA1_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
		BCryptCreateHash(hAlg, &hHash, NULL, 0,
			(PUCHAR)SECRET, (ULONG)(sizeof(SECRET) - 1), 0);
		BCryptHashData(hHash, (PUCHAR)message.data(), (ULONG)message.size(), 0);

		const DWORD SHA1_LEN = 20;
		std::vector<unsigned char> hash(SHA1_LEN);
		BCryptFinishHash(hHash, hash.data(), SHA1_LEN, 0);
		BCryptDestroyHash(hHash);
		BCryptCloseAlgorithmProvider(hAlg, 0);

		DWORD b64Len = 0;
		CryptBinaryToStringA(hash.data(), SHA1_LEN,
			CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &b64Len);
		std::string b64(b64Len, '\0');
		CryptBinaryToStringA(hash.data(), SHA1_LEN,
			CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &b64[0], &b64Len);
		while (!b64.empty() && b64.back() == '\0') b64.pop_back();

		return b64 + "\n";
	}

	// Today's date as YYYYMMDD (UTC).
	static inline std::string Date()
	{
		SYSTEMTIME st; GetSystemTime(&st);
		char buf[9];
		snprintf(buf, sizeof(buf), "%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
		return buf;
	}

	// Current time as ISO 8601 (UTC).
	static inline std::string Timestamp()
	{
		SYSTEMTIME st; GetSystemTime(&st);
		char buf[25];
		snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		return buf;
	}

	// Cryptographically random UUID v4.
	static inline std::string Guid()
	{
		unsigned char b[16] = {};
		HCRYPTPROV prov = 0;
		if (CryptAcquireContextA(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		{
			CryptGenRandom(prov, sizeof(b), b);
			CryptReleaseContext(prov, 0);
		}
		b[6] = (b[6] & 0x0F) | 0x40; // version 4
		b[8] = (b[8] & 0x3F) | 0x80; // variant bits
		char buf[37];
		snprintf(buf, sizeof(buf),
			"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			b[0],b[1],b[2],b[3], b[4],b[5], b[6],b[7],
			b[8],b[9], b[10],b[11],b[12],b[13],b[14],b[15]);
		return buf;
	}

	// Appends &signature=...&signature_protocol=sha1 to the URL.
	static inline std::string SignUrl(const std::string& url)
	{
		return url
			+ "&signature=" + UrlEncode(Sign(url + Date()))
			+ "&signature_protocol=sha1";
	}

	static inline std::unordered_map<std::string, std::string> Headers()
	{
		return { { "Cookie", COOKIE }, { "User-Agent", UA } };
	}
}

class MusixmatchDecoder : public Decoder
{
public:
	// Cached anonymous usertoken. Persisted to s_plugin_dir\musixmatch_token.txt.
	inline static std::string s_usertoken;
	// Set by Plugin.cpp (ReadSettingsFile) so the token file path is known.
	inline static std::string s_plugin_dir;

	bool DecodeLyrics(const std::string& artist, const std::string& song,
	                  MajestLyrics::Album& out_album) override
	{
		if (!EnsureToken()) return false;

		// Order: app_id → usertoken → format → endpoint params (matches Rust new_url + append order).
		const std::string url = MxmApi::SignUrl(
			std::string(MxmApi::BASE_URL) + "matcher.lyrics.get"
			"?app_id="     + std::string(MxmApi::APP_ID)   +
			"&usertoken="  + UrlEncode(s_usertoken)         +
			"&format=json"
			"&q_track="    + UrlEncode(song)                +
			"&q_artist="   + UrlEncode(artist));

		std::string data = HttpClient::Get(url, false, MxmApi::Headers());
		if (data == "failed" || data == "failed_tls") return false;

		// A 401 means the cached token expired — clear it for the next attempt.
		if (data.find("\"status_code\":401") != std::string::npos)
		{
			s_usertoken.clear();
			return false;
		}

		std::string lyrics = ExtractJsonString(data, "lyrics_body");
		if (lyrics.empty()) return false;

		// Strip the Musixmatch commercial-use footer (free-tier only).
		const std::string footer = "\n\n******* This Lyrics is NOT for Commercial use *******";
		auto footerPos = lyrics.find(footer);
		if (footerPos != std::string::npos) lyrics.resize(footerPos);
		while (!lyrics.empty() && (lyrics.back() == '\n' || lyrics.back() == '\r'))
			lyrics.pop_back();
		if (lyrics.empty()) return false;

		out_album.name = UTF8ToWide(artist);
		std::wstring wideLyrics = UTF8ToWide(lyrics);
		StripCR(wideLyrics);
		out_album.songs[SongKey(UTF8ToWide(artist), UTF8ToWide(song))] = std::move(wideLyrics);
		return true;
	}

private:
	void Parse(const std::string&, MajestLyrics::Album&) override {}

	bool EnsureToken()
	{
		if (!s_usertoken.empty()) return true;

		// Try loading a previously cached token from disk.
		if (!s_plugin_dir.empty())
		{
			std::ifstream f(s_plugin_dir + "\\musixmatch_token.txt");
			if (f && std::getline(f, s_usertoken) && !s_usertoken.empty())
				return true;
		}

		return FetchToken();
	}

	bool FetchToken()
	{
		// token.get is rate-limited to 2 req/min — only called when no valid token exists.
		const std::string ts   = MxmApi::Timestamp();
		const std::string advId = MxmApi::Guid();
		const std::string guid  = MxmApi::Guid();

		const std::string url = MxmApi::SignUrl(
			std::string(MxmApi::BASE_URL) + "token.get"
			"?adv_id="      + UrlEncode(advId)              +
			"&root=0"
			"&sideloaded=0"
			"&app_id="      + std::string(MxmApi::APP_ID)   +
			"&build_number="+ std::string(MxmApi::BUILD)    +
			"&guid="        + UrlEncode(guid)                +
			"&lang=en_US"
			"&model="       + UrlEncode(MxmApi::MODEL)       +
			"&timestamp="   + UrlEncode(ts)                  +
			"&format=json");

		std::string data = HttpClient::Get(url, false, MxmApi::Headers());
		if (data == "failed" || data == "failed_tls") return false;

		s_usertoken = ExtractJsonString(data, "user_token");
		if (s_usertoken.empty()) return false;

		// Persist for future plugin sessions.
		if (!s_plugin_dir.empty())
		{
			std::ofstream f(s_plugin_dir + "\\musixmatch_token.txt");
			if (f) f << s_usertoken;
		}

		return true;
	}
};

#endif
