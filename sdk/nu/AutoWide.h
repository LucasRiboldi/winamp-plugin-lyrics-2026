#ifndef AUTOWIDEH
#define AUTOWIDEH

#include <windows.h>

inline wchar_t *AutoWideDup(const char *convert, UINT codePage = CP_ACP)
{
	wchar_t *wide = 0;
	if (convert)
	{
		const int size = MultiByteToWideChar(codePage, 0, convert, -1, 0, 0);
		if (size > 0)
		{
			wide = (wchar_t *)malloc(size << 1);
			if (!MultiByteToWideChar(codePage, 0, convert, -1, wide, size))
			{
				free(wide);
				wide = 0;
			}
			else
			{
				wide[size - 1] = 0;
			}
		}
	}
	return wide;
}

class AutoWide
{
public:
	AutoWide(const char *convert, UINT codePage = CP_ACP) : wide(0)
	{
		wide = AutoWideDup(convert, codePage);
	}

	~AutoWide()
	{
		free(wide);
		wide = 0;
	}

	operator wchar_t *() const
	{
		return wide;
	}

private:
	wchar_t *wide;
};

#endif