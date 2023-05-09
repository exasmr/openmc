#pragma once

#include <cstring> // strcpy
#include <string>

#include <fmt/format.h>

#include "openmc/error.h"

// Mixin class that gives something a statically sized name.
// This is useful because it lets the name be trivially copied,
// which means that we're going to be able to safely access the
// name from GPU, and can simply realloc data containing this.
// That's not the case with std::string due to short string
// optimization.
namespace openmc {

template<int MaxNameLength>
class Named {
public:
  Named()
  {
    static_assert(MaxNameLength > 0,
      "must have at least one character for null terminator");
    name_[0] = '\0';
  }

  void set_name(std::string const& name)
  {
    if (name.size() + 1 >= MaxNameLength)
      fatal_error(
        fmt::format("Trying to set a name which has a max size of {}, "
                    "but passed in {} which is too long.",
          MaxNameLength - 1, name));
    else {
      std::strcpy(name_, name.c_str());
    }
  }

  std::string name() const { return name_; }

  const char* name_data() const { return name_; }

private:
  char name_[MaxNameLength];
};

} // namespace openmc
