#pragma once
#include <string>
#include <core/Album.h>

class Decoder
{
public:
	virtual bool DecodeLyrics(
		const std::string& artist,
		const std::string& song,
		MajestLyrics::Album& out_album) = 0;
protected:
	virtual void Parse(const std::string& data, MajestLyrics::Album& out_album) = 0;
};
