#pragma once
#ifndef _MAJEST_STRING_UTIL_
#define _MAJEST_STRING_UTIL_
#include <type_traits>
#include <string>
#include <vector>
#include <algorithm>

static std::wstring ToLower(const std::wstring& s)
{
	std::wstring t{ s };
	std::transform(t.begin(), t.end(), t.begin(), [](wchar_t c) { return std::tolower(c); });
	return t;
}

static std::string ToLower(const std::string& s)
{
	std::string t{ s };
	std::transform(t.begin(), t.end(), t.begin(), [](char c) { return std::tolower(c); });
	return t;
}

static std::string UrlEncode(const std::string& str)
{
	std::string out;
	out.reserve(str.size() * 3);
	for (unsigned char c : str)
	{
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			out += (char)c;
		else
		{
			char buf[4];
			snprintf(buf, sizeof(buf), "%%%02X", c);
			out += buf;
		}
	}
	return out;
}

static inline std::vector<std::string> Split(const std::string& str, const std::string separator) noexcept
{
	std::vector<std::string> result;
	if (str.empty()) return result;
	size_t offset = 0, tmp = 0;
	try
	{
		do
		{
			tmp = offset;
			offset = str.find_first_of(separator, offset);
			if (offset == std::string::npos) offset = str.size();
			offset += 1;
			std::string s = str.substr(tmp, offset - tmp - 1);
			if (!s.empty() && s[0] == ' ')
			{
				size_t ns = s.find_first_not_of(' ');
				if (ns != std::string::npos) s = s.substr(ns);
			}
			if (s.size() > 1 && s.back() == ' ') s.pop_back();
			result.push_back(s);
		} while (offset < str.size());
	}
	catch (...) { return { "except" }; }
	return result;
}

#endif
