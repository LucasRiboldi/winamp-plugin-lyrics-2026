#ifndef NULLSOFT_AUTOCHARH
#define NULLSOFT_AUTOCHARH

#include <windows.h>

inline int AutoCharSize(const wchar_t *convert, size_t len, UINT codePage = CP_ACP, UINT flags = 0)
{
	return (convert ? WideCharToMultiByte(codePage, flags, convert, (int)len, 0, 0, NULL, NULL) : 0);
}

inline char *AutoCharDupN(const wchar_t *convert, size_t len, UINT codePage = CP_ACP, UINT flags = 0)
{
	const int size = AutoCharSize(convert, len, codePage, flags);
	
	if (!size)
	{
		return 0;
	}

	char *narrow = (char *)malloc((size + 1));
	if (0 == narrow)
	{
		return 0;
	}

	if (!WideCharToMultiByte(codePage, flags, convert, (int)len, narrow, size, NULL, NULL))
	{
		free(narrow);
		narrow = 0;
	}
	else
	{
		narrow[size - 1] = 0;
	}
	return narrow;
}

inline char *AutoCharDup(const wchar_t *convert, UINT codePage = CP_ACP, UINT flags = 0)
{
	const int size = AutoCharSize(convert, (size_t)-1, codePage, flags);

	if (!size)
	{
		return 0;
	}

	char *narrow = (char *)malloc(size);
	if (!WideCharToMultiByte(codePage, flags, convert, -1, narrow, size, NULL, NULL))
	{
		free(narrow);
		narrow = 0;
	}
	else
	{
		narrow[size - 1] = 0;
	}
	return narrow;
}

class AutoChar
{
public:
	AutoChar(const wchar_t *convert, UINT codePage = CP_ACP, UINT flags = 0) : narrow(0)
	{
		narrow = AutoCharDup(convert, codePage, flags);
	}

	~AutoChar()
	{
		free(narrow);
		narrow = 0;
	}

	operator const char *() const
	{
		return narrow;
	}

	operator char *() const
	{
		return narrow;
	}

protected:
	AutoChar() : narrow(0)
	{
	}
	char *narrow;
};

class AutoCharGrow
{
public:
	AutoCharGrow()
	{
		narrow = 0;
		size = 0;
	}

	~AutoCharGrow()
	{
		free(narrow);
	}

	const char *Convert(const wchar_t *convert, UINT codePage = CP_ACP, UINT flags = 0, size_t *cch = 0)
	{
		const int new_size = AutoCharSize(convert, (size_t)-1, codePage, flags);

		if (!new_size)
		{
			return 0;
		}

		if ((size_t)new_size > size)
		{
			free(narrow);
			narrow = (char *)malloc(new_size);
			if (!narrow)
			{
				size = 0;
				return 0;
			}
			size = (size_t)new_size;
		}

		if (!WideCharToMultiByte(codePage, flags, convert, -1, narrow, new_size, NULL, NULL))
		{
			return 0;
		}

		if (cch)
		{
			*cch = new_size - 1;
		}
		return narrow;
	}

protected:
	char *narrow;
	size_t size;
};

class AutoCharN : public AutoChar
{
public:
	AutoCharN(const wchar_t *convert, size_t len, UINT codePage = CP_ACP, UINT flags = 0)
	{
		narrow = AutoCharDupN(convert, len, codePage, flags);
	}
};

#endif