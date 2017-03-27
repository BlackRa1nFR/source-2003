
// The VString class is useful to use for variable-length strings but is NOT
// good for any runtime string operations.

#ifndef VSTRING_H
#define VSTRING_H


class VString
{
public:
			VString();
			~VString();

	inline	operator const char*()	const {return m_pStr;}

	// Comparisons.
	int		Strcmp(const char *pStr);
	int		Stricmp(const char *pStr);

	// Use to copy the string data.
	bool	CopyString(const char *pStr);
	void	FreeString();


private:
	char	*m_pStr;
};


#endif


