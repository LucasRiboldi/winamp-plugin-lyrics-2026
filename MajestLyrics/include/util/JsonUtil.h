#pragma once
#ifndef _MAJEST_JSON_UTIL_
#define _MAJEST_JSON_UTIL_
#include <string>

// Extract the string value of a JSON key from a flat or nested JSON blob.
// Handles the common escape sequences (\n \r \t \" \\ \uXXXX).
// Returns "" if the key is absent or if the value exceeds MAX_LYRICS_BYTES.
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

#endif
