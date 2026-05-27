#pragma once
#ifndef _MAJEST_LYRIC_HANDLER_
#define _MAJEST_LYRIC_HANDLER_
#include <net/HttpClient.h>
#include <util/StringUtil.h>
#include <decoders/LyricDecoder.h>
#include <core/Album.h>
#include <string>
#include <unordered_map>
#include <type_traits>

template <typename F>
struct is_function_ptr
	: std::integral_constant<bool,
		std::is_pointer<F>::value &&
		std::is_function<typename std::remove_pointer<F>::type>::value>
{};

class LyricHandler
{
public:
	LyricHandler();
	~LyricHandler() = default;

	template<class Fn>
	typename std::enable_if<is_function_ptr<Fn>::value>::type
	GetLyrics(const std::string& artist, const std::string& song, Fn decoder)
	{
		decoder(artist, song, album);
	}

	std::wstring& operator[](std::wstring& s);
	std::wstring& operator[](std::string& s);
	std::wstring& operator[](const wchar_t* s);
	std::wstring& operator[](const char* s);

	const std::pair<std::wstring, bool> GetInterval(
		const std::wstring& song, int start, int count) const;

	const MajestLyrics::Album& GetAlbum() const noexcept;
	size_t                      GetSize()  const noexcept;
	int GetLineCount(const std::string&  s) const noexcept;
	int GetLineCount(const std::wstring& s) const noexcept;

	bool HasSong(const std::wstring& s) const noexcept
	{
		return album.songs.count(ToLower(s)) > 0;
	}

	std::wstring GetSong(const std::wstring& s) const noexcept
	{
		auto it = album.songs.find(ToLower(s));
		return it != album.songs.end() ? it->second : L"";
	}

	void ClearSong(const std::wstring& s) noexcept
	{
		album.songs.erase(ToLower(s));
	}

	friend std::wostream& operator<<(std::wostream& os, const LyricHandler& lh);

protected:
	MajestLyrics::Album album;
};

#endif
