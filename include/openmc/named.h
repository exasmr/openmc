#pragma once

#include <cstdlib>
#include <cstring> // strcpy
#include <string>

#include <fmt/format.h>

#include "openmc/error.h"

// Mixin class that gives something a name, represented as methods
// called name, name_data, and name_empty.
// This is useful because it lets the name be trivially copied,
// so stuff doesn't break when memcpying data around to the GPU.
// That's not the case with std::string due to short string
// optimization.
namespace openmc {

class Named {
public:
  Named() = default;

  Named(Named&& other)
  {
    name_ = other.name_;
    other.name_ = nullptr;
  }

  Named& operator=(Named&& other)
  {
    name_ = other.name_;
    other.name_ = nullptr;
    return *this;
  }

  Named(const Named& other)
  {
    if (other.name_) {
      auto len = std::strlen(other.name_);
      name_ = (char*)std::malloc(len + 1);
      std::strcpy(name_, other.name_);
      name_[len] = '\0';
    }
  }

  Named& operator=(const Named& other)
  {
    free();
    if (other.name_) {
      auto len = std::strlen(other.name_);
      name_ = (char*)std::malloc(len + 1);
      std::strcpy(name_, other.name_);
      name_[len] = '\0';
    }
    return *this;
  }

  ~Named() { free(); }

  void set_name(std::string const& name)
  {
    if (name.empty()) {
      free();
    } else {
      name_ = (char*)std::realloc(name_, name.size() + 1);
      std::strcpy(name_, name.c_str());
      name_[name.size()] = '\0';
    }
  }

  const std::string name() const
  {
    if (name_)
      return std::string(name_);
    else
      return "";
  }
  const char* name_data() const { return name_; }
  bool name_empty() const { return name_ == nullptr; }

private:
  char* name_ {nullptr};

  void free()
  {
    if (name_) {
      std::free(name_);
      name_ = nullptr;
    }
  }
};

} // namespace openmc
