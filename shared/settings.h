#pragma once

#include <string>

namespace csnet
{
namespace shared
{

// setting provider class
// read and write setting values
class settings_provider_t
{
public:
	// read value by 'name' from 'section'
    virtual std::string get_value(const std::string& section, const std::string& name) const = 0;
	// write value by 'name' from 'section'
	virtual std::string set_value(const std::string& section, const std::string& name, const std::string& value) = 0;

    virtual ~settings_provider_t() = default;
};

// store settings base class
class settings_t
{
protected:
    settings_t(settings_provider_t* provider) : _provider(provider)
    {
    }
    virtual ~settings_t()
    {
    }
    
public:
    // load settings
    virtual void load() = 0;
    // save settings
    virtual void save() = 0;
    // set default values
    virtual void reset() = 0;
    
protected:
	// get value by 'name' from 'section'
	std::string get_value(const std::string& section, const std::string& name) const
    {
        return _provider->get_value(section, name);
    }
	
	// set value by 'name' from 'section'
    void set_value(const std::string& section, const std::string& name, const std::string& value)
    {
        _provider->set_value(section, name, value);
    }
    
protected:
    settings_provider_t* _provider = nullptr;
};

}
}
