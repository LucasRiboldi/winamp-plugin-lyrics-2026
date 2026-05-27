#include <core/LyricHandler.h>
#include <util/StringUtil.h>
#include <sstream>
#include <algorithm>
#include <vector>

LyricHandler::LyricHandler() : album{} {}

std::wstring& LyricHandler::operator[](std::wstring& s) { return album.songs[ToLower(s)]; }
std::wstring& LyricHandler::operator[](std::string& s)  { return album.songs[ToLower(UTF8ToWide(s))]; }
std::wstring& LyricHandler::operator[](const wchar_t* s) { return album.songs[ToLower(std::wstring(s))]; }
std::wstring& LyricHandler::operator[](const char* s)    { return album.songs[ToLower(UTF8ToWide(s))]; }

const MajestLyrics::Album& LyricHandler::GetAlbum() const noexcept { return album; }
size_t LyricHandler::GetSize() const noexcept { return album.songs.size(); }

const std::pair<std::wstring, bool> LyricHandler::GetInterval(
	const std::wstring& song, int start, int count) const
{
	auto it = album.songs.find(ToLower(song));
	if (it == album.songs.end()) return { L"", true };

	std::vector<std::wstring> lines;
	std::wstringstream ss(it->second);
	std::wstring line;
	while (std::getline(ss, line)) lines.push_back(line);

	int total = (int)lines.size();
	int end   = std::min(total, start + count);
	std::wstring result;
	for (int i = start; i < end; i++) result += lines[i] + L"\n";

	return { result, (start + count) >= total };
}

int LyricHandler::GetLineCount(const std::string& s) const noexcept
{
	return GetLineCount(UTF8ToWide(s));
}

int LyricHandler::GetLineCount(const std::wstring& s) const noexcept
{
	auto it = album.songs.find(ToLower(s));
	if (it == album.songs.end()) return 0;
	return (int)std::count(it->second.begin(), it->second.end(), L'\n');
}

std::wostream& operator<<(std::wostream& os, const LyricHandler& lh)
{
	for (const auto& [title, lyrics] : lh.album.songs)
		os << title << L"\n" << lyrics << L"\n\n";
	return os;
}
