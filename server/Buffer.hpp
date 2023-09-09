#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <sys/_types/_size_t.h>

class Buffer {
public:
	static const size_t	capacity = 1000000;
private:
	char	buf[capacity];
	size_t	size;
//Buffer.cpp
public:
	Buffer();
	Buffer(const Buffer& src);
	~Buffer();
	Buffer&	operator=(const Buffer& src);
public:
	const char*	getBuf(void) const;
	size_t		getSize(void) const;
	void		setSize(const size_t newSize);
	char*		begin(void);
	const char*	begin(void) const;
	char*		end(void);
	const char* end(void) const;
};

#endif