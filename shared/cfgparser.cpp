#include <unistd.h>
#include <limits.h>
#include <cstdlib>
#include <libgen.h>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <vector>
#include <array>

#include "cfgparser.h"

namespace csnet
{
namespace shared
{

static constexpr int BUFFER_SIZE = 1024; // buffer size to read
    
// init and open a config file
cfgparser_t::cfgparser_t(const std::string& filename)
{
    openfile(filename);
}

// do default
cfgparser_t::~cfgparser_t()
{
}

// open config file
void cfgparser_t::openfile(const std::string& filename)
{
    std::string fn = findfile(filename);
	_file.open(fn, std::ios::in);
	if (!_file)
	{
		std::stringstream buf;
		buf << "Cannot open config file \"" << filename << "\"";
		throw std::runtime_error(buf.str());
	}
}

// find config file
std::string cfgparser_t::findfile(const std::string& filename) const
{
    std::string cfg = filename;
    // if file exists use specified file name
    if (access(cfg.c_str(), F_OK) == -1)
    {
        //file not found try to find it
        
        std::array<char, PATH_MAX> dest;
        if (readlink("/proc/self/exe", dest.data(), PATH_MAX) != -1) // get process full path
        {
            // process name + '.cfg' OR cfg name w/o path
            std::string fn = cfg.empty() ? (basename(dest.data()) + std::string(".cfg")) : basename(&cfg.front());
            // process path w/o name
            std::string dn = dirname(dest.data());
            
            //try to find it in the current dir
            cfg = fn;
            if (access(cfg.c_str(), F_OK) != -1)
                return cfg;
            
            //try to find it in the process path
            cfg = dn;
            cfg += "/" + fn;
            if (access(cfg.c_str(), F_OK) != -1)
                return cfg;
            
            //try to find it in cfg dir
            cfg = dn;
            cfg += "/../cfg/";
            cfg += fn;
            if (access(cfg.c_str(), F_OK) != -1)
                return cfg;
        }
    }
    return cfg;
}

// read value by 'name' from 'section'
std::string cfgparser_t::get_value(const std::string& section, const std::string& name) const
{
    _file.clear(); // clear file state

	if (find_section("[" + section + "]"))
	{
        while (!_file.eof())
		{
            // read chunk to buffer 
            std::array<char, BUFFER_SIZE> buf;
            _file.getline(buf.data(), BUFFER_SIZE);
            std::string line = buf.data();
            
			if (is_section(line))
                break; // new section after empty section, there are no any data to read in specified section
            
            line = trim(line);
            if (!line.empty())
            {
                //is there 'name = value' expression?
                int pos = line.find("=");
                if (pos && pos != -1)
                {
                    // yes, extract the value
                    std::string n = trim(line.substr(0, pos));
                    if (n.size() >= name.size() && compare_wo_space(n, name))
                        return trim(line.substr(pos + 1));
                }
            }
		}
	}
	return "";
}

// NOT IMPLEMENTED
std::string cfgparser_t::set_value(const std::string& section, const std::string& name, const std::string& value)
{
    throw std::runtime_error("cfgparser_t::set_value is not implemented");
}

// find section like '[section]' in the config file
bool cfgparser_t::find_section(const std::string& section) const
{
    // set read file pointer to begin
	_file.seekg(0, std::ios_base::beg);

	while (!_file.eof())
	{
        // read chunk to buffer 
        std::array<char, BUFFER_SIZE> buf;
		_file.getline(buf.data(), BUFFER_SIZE);
        std::string line = buf.data();

		// compare the line with specified section name
        if (line.size() >= section.size() && compare_wo_space(line, section))
        {
			return true; // section is found
        }
	}
	return false;
}

// compare string w/o space like ' s ec ti   n' is equ 'section'
bool cfgparser_t::compare_wo_space(const std::string& str1, const std::string& str2) const
{
	const char* p1 = str1.c_str();
	const char* p2 = str2.c_str();

	// skip all space char and compare other char
    do
	{
		if (*p1 == '\0')
			break;

		if (std::isspace(*p1))
			continue;

		if (*p2 == '\0')
			break;

		if (std::toupper(*p1) != std::toupper(*p2))
			return false;
        
        ++p2;
	}
	while (++p1);

	return *p1 == '\0' && *p2 == '\0';
}

// is str a section like '[section]'
bool cfgparser_t::is_section(const std::string& str) const
{
	const char* p = str.c_str();;

	// skip all space char and compare other char
	do
	{
		if (*p == '\0')
			break;

		if (std::isspace(*p))
			continue;

		if (*p == '[')
		{
			do
			{
				if (*p == '\0')
					break;

				if (*p == ']')
					return true;
			}
			while (++p);
		}
		else
		{
			return false;
		}
	}
	while (++p);
	return false;
}

// left trim string
std::string cfgparser_t::ltrim(const std::string& str) const
{
    if (str.empty())
        return str;
    
    std::string temp = str;
	const char* p = temp.c_str(); // pointer to begin of the string
    
	while(*p && std::isspace(*p))
	{
		++p;
	}
    
	if (*p)
	{
		temp.erase(0, p - temp.c_str()); // erase space chars
	}
	return temp;
}

// right trim string
std::string cfgparser_t::rtrim(const std::string& str) const
{
    if (str.empty())
        return str;

    std::string temp = str;
	const char* p = temp.c_str();
	p += temp.size() - 1; // pointer to end of the string
    
	while(*p && std::isspace(*p))
	{
		temp.erase(p - temp.c_str(), -1); // erase space chars
        
        p = temp.c_str();
        p += temp.size() - 1; // pointer to end of the string
	}
	return temp;
}

// trim string
std::string cfgparser_t::trim(const std::string& str) const
{
	std::string temp = ltrim(str);
	return rtrim(temp);
}

}
}
