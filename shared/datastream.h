#pragma once

#include <unistd.h>
#include <streambuf>
#include <sstream>
#include <vector>
#include <algorithm>

class bytebuf : public std::basic_streambuf<int8_t>
{
public:
	bytebuf()
	{
		setp(_obuf, _obuf + sizeof(_obuf) - 1);
		setg(_ibuf, _ibuf, _ibuf);
	}
	virtual ~bytebuf() 
    { 
        sync(); 
    }

protected:
	int output_buffer()
	{
       if (pptr() < pbase())
          return 0;
      
		int num = pptr() - pbase();
        _buf.insert(_buf.end(), pbase(), pbase() + num);
        
		pbump(-num);
		return num;
	}
	virtual int_type overflow(int_type c)
	{
		/*if (c == traits_type::eof())
            return output_buffer();

        *pptr() = c;
        pbump(1);
        
        if (pptr() >= epptr())
        {
            if(output_buffer() == traits_type::eof())
                return traits_type::eof();
        }
		return c;*/
        
		if (c != traits_type::eof())
		{
			*pptr() = c;
			pbump(1);
		}

		if (output_buffer() == traits_type::eof())
			return traits_type::eof();
		return c;
	}
	virtual int sync()
	{
		if(output_buffer() == traits_type::eof())
			return static_cast<int>(traits_type::eof());
		return 0;
	}
	virtual int_type underflow()
	{
		if (gptr() < egptr())
			return *gptr();

        if (_index >= _buf.size())
			return traits_type::eof();
        
        size_t num = std::min(sizeof(_ibuf), _buf.size() - _index);
        std::copy(_buf.begin() + _index, _buf.begin() + _index + num, eback());
        
		_index += num;
        setg(eback(), eback(), eback() + num);
		return *gptr();
	}
    
protected:
    static const size_t _buffsize = 128;
	int8_t _obuf[_buffsize];
	int8_t _ibuf[_buffsize];

	std::vector<int8_t> _buf;
    size_t _index = 0;
};

typedef std::basic_iostream<int8_t> datastream;
/*class datastream : public std::basic_iostream<int8_t>
{
protected:
	bytebuf _buf;

public:
	basic_socketstream(): stream_type(&_buf) {}
};*/

