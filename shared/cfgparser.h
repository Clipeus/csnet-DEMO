#pragma once

#include <string>
#include <fstream>
#include "settings.h"

namespace csnet
{
  namespace shared
  {

    // cfg file reader class
    class cfgparser_t : public settings_provider_t
    {
    public:
      // init and open a config file
      explicit cfgparser_t(const std::string& filename);
      // do default
      virtual ~cfgparser_t();

    public:
      // read value by 'name' from 'section'
      std::string get_value(const std::string& section, const std::string& name) const;
      // NOT IMPLEMENTED
      std::string set_value(const std::string& section, const std::string& name, const std::string& value);

    private:
      // find section like '[section]' in the config file
      bool find_section(const std::string& section) const;
      // compare string w/o space like ' s ec ti   n' is equ 'section'
      bool compare_wo_space(const std::string& str1, const std::string& str2) const;
      // is str a section like '[section]'
      bool is_section(const std::string& str) const;
      // left trim string
      std::string ltrim(const std::string& temp) const;
      // right trim string
      std::string rtrim(const std::string& temp) const;
      // trim string
      std::string trim(const std::string& temp) const;
      // open config file
      void openfile(const std::string& filename);
      // find config file
      std::string findfile(const std::string& filename) const;

    private:
      mutable std::fstream _file;
    };

  }
}
