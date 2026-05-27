#pragma once
#ifndef _MAJEST_HTTP_CLIENT_
#define _MAJEST_HTTP_CLIENT_
#include <string>
#include <unordered_map>

class HttpClient
{
public:
	static std::string Get(
		const std::string& url,
		bool redirect = false,
		const std::unordered_map<std::string, std::string>& headers = {});
};

#endif
