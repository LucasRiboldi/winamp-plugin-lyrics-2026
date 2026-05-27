#include <net/HttpClient.h>
#include <Windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#include <vector>

static std::wstring ToWide(const std::string& s)
{
	if (s.empty()) return L"";
	int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
	if (size <= 0) return L"";
	std::wstring result(size, 0);
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &result[0], size);
	return result;
}

static void ParseURL(const std::string& url,
                     std::string& host, std::string& path,
                     INTERNET_PORT& port, bool& secure)
{
	secure = (url.size() >= 8 && url.substr(0, 8) == "https://");
	port   = secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

	size_t start = url.find("://");
	if (start == std::string::npos) { host = url; path = "/"; return; }
	start += 3;

	size_t slash = url.find('/', start);
	if (slash == std::string::npos) { host = url.substr(start); path = "/"; }
	else { host = url.substr(start, slash - start); path = url.substr(slash); }

	size_t colon = host.find(':');
	if (colon != std::string::npos)
	{
		try { port = (INTERNET_PORT)std::stoi(host.substr(colon + 1)); } catch (...) {}
		host = host.substr(0, colon);
	}
}

std::string HttpClient::Get(const std::string& url,
                             bool /*redirect*/,
                             const std::unordered_map<std::string, std::string>& headers)
{
	std::string host, path;
	INTERNET_PORT port;
	bool secure;
	ParseURL(url, host, path, port, secure);

	HINTERNET hSession = WinHttpOpen(L"MajestLyrics/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) return "failed";

	HINTERNET hConnect = WinHttpConnect(hSession, ToWide(host).c_str(), port, 0);
	if (!hConnect) { WinHttpCloseHandle(hSession); return "failed"; }

	HINTERNET hRequest = WinHttpOpenRequest(
		hConnect, L"GET", ToWide(path).c_str(),
		nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
		secure ? WINHTTP_FLAG_SECURE : 0);
	if (!hRequest)
	{
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return "failed";
	}

	for (const auto& h : headers)
	{
		std::wstring header = ToWide(h.first + ": " + h.second + "\r\n");
		WinHttpAddRequestHeaders(hRequest, header.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);
	}

	bool sent = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
	                               WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	if (!sent || !WinHttpReceiveResponse(hRequest, nullptr))
	{
		DWORD err = GetLastError();
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return (err == ERROR_WINHTTP_SECURE_FAILURE) ? "failed_tls" : "failed";
	}

	std::string response;
	DWORD dwSize = 0;
	do
	{
		if (!WinHttpQueryDataAvailable(hRequest, &dwSize) || dwSize == 0) break;
		std::vector<char> buf(dwSize + 1, '\0');
		DWORD dwRead = 0;
		if (!WinHttpReadData(hRequest, buf.data(), dwSize, &dwRead)) break;
		response.append(buf.data(), dwRead);
	} while (true);

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
	return response.empty() ? "failed" : response;
}
