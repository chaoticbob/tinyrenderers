#ifndef __cplusplus
  #error "C++ is required"
#endif

#ifndef TINY_RENDERR_FS_PATH_H
#define TINY_RENDERR_FS_PATH_H

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace tr {
namespace fs {

class path {
public:
  path() {}

  path(const std::string& s) { 
    append(s, true); 
  }

  path(const char* s) { 
    append(s, true); 
  }

  path(const path& p) { 
    m_cached = p.m_cached;
    m_dirty = p.m_dirty;
    m_has_root = p.m_has_root; 
    m_parts = p.m_parts; 
  }

  virtual ~path() {}

  operator bool() const { 
    update_cache(); 
    return m_cached.empty() ? false : true; 
  }

  operator std::string() const { 
    return str(); 
  }

  bool  operator==(const path& rhs) const { 
    return str() == rhs.str(); 
  }

  bool  operator!=(const path& rhs) const { 
    return str() != rhs.str(); 
  }

  path& operator=(const path& rhs) { 
    if (this != &rhs) { 
      m_cached = rhs.m_cached;
      m_dirty = rhs.m_dirty; 
      m_has_root = rhs.m_has_root; 
      m_parts = rhs.m_parts; 
    } 
    return *this; 
  }

  path& operator=(const std::string& rhs) { 
    append(rhs, true); 
    return *this; 
  }

  path& operator=(const char* rhs) { 
    append(rhs, true); 
    return *this; 
  }

  path& operator/=(const path& rhs) { 
    append(rhs.str()); 
    return *this; 
  }

  path& operator/=(const std::string& rhs) { 
    append(rhs); 
    return *this; 
  }

  path& operator/=(const char* rhs) { 
    append(rhs); 
    return *this; 
  }

  path  operator/(const path& rhs) const { 
    path r = *this;
    r.append(rhs.str()); 
    return r; 
  }

  path  operator/(const std::string& rhs) const { 
    path r = *this; 
    r.append(rhs); 
    return r; 
  }

  path  operator/(const char* rhs) const { 
    path r = *this; 
    r.append(rhs); 
    return r; 
  }

  const std::string&  str() const { 
    update_cache(); 
    return m_cached; 
  }

  const char* c_str() const { 
    update_cache(); 
    return m_cached.c_str(); 
  }

  bool  is_root() const { 
    update_cache(); 
    return ((m_cached.size() == 1) && (m_cached[0] == '/')) || 
            ((m_cached.size() == 2) && (m_cached[1] == ':'));
  }

  fs::path parent() const { 
    fs::path r = *this; 
    if ((! r.is_root()) && (! r.m_parts.empty())) { 
      r.m_parts.pop_back(); 
    }
    r.m_cached.clear(); 
    r.m_dirty = true; 
    return r; 
  }
  
  /*! @fn extension 

   @return Returns the extension start from, and including, the last period.

   file.ext       - ".ext" is returned
   file.other.ext - ".ext" is returned

  */
  fs::path extension() const {
    fs::path ext;
    if (!m_parts.empty()) {
      const std::string& s = m_parts.back();
      std::string::size_type pos = s.rfind('.');
      if (pos != std::string::npos) {        
        std::string::size_type len = s.length();
        std::string s_ext = s.substr(pos, len - pos);
        ext = fs::path(s_ext);
      }
    }
    return ext;
  }


  /*! @fn full_extension 

   @return Returns the extension start from, and including, the last period.

   file.ext       - ".ext" is returned
   file.other.ext - "other.ext" is returned

  */
  fs::path full_extension() const {
    fs::path ext;
    if (!m_parts.empty()) {
      const std::string& s = m_parts.back();
      std::string::size_type pos = s.find('.');
      if (pos != std::string::npos) {        
        std::string::size_type len = s.length();
        std::string s_ext = s.substr(pos, len - pos);
        ext = fs::path(s_ext);
      }
    }
    return ext;
  }

private:
  void append(std::string s, bool reset = false) {
    if (s.empty()) { 
      return; 
    }

    if ((! reset) && (s[0] == '/')) {
      throw std::runtime_error("cannot append path that contains root");
    }

    // Change all '\' to '/'
    if (s.find('\\') != std::string::npos) {
      std::transform(s.begin(), 
                     s.end(), 
                     s.begin(),
                     [](typename std::string::value_type c)
                         { return (c == '\\') ? '/' : c; });
    }

    // Collapse repeating '/' to a single '/'
    while (s.find("//") != std::string::npos) {
      auto new_end = std::unique(s.begin(), 
                                 s.end(), 
                                 [](typename std::string::value_type lhs, 
                                    typename std::string::value_type rhs) 
                                        { return (lhs == rhs) && (lhs == '/'); });
      s.erase(new_end, s.end());
    }

    // Clear everything if this is a reset
    if (reset) {
      m_cached.clear(); 
      m_dirty = false; 
      m_has_root = false;  
      m_parts.clear(); 
    }

    // Split string into components
    m_has_root = (! s.empty()) && ((s[0] == '/') || ((s.size() > 1) && (s[1] == ':')));
    if (s.find('/') != std::string::npos) {
      std::istringstream is(s);
      while (std::getline(is, s, '/')) { 
        m_parts.push_back(s); 
      }
    } else {
      m_parts.push_back(s);
    }
    // Mark dirty
    m_dirty = true;
  }

  void update_cache() const {
    if (! m_dirty) { 
      return; 
    }

    m_cached.clear();
    
    if (m_has_root && (! ((m_parts[0].size() > 1) && (m_parts[0][1] == ':')))) { 
        m_cached = "/"; 
    }

    if (! m_parts.empty()) {
      auto iter = m_parts.begin();
      m_cached = *(iter++);
      while (iter != m_parts.end()) {
        m_cached += '/';
        m_cached += *(iter++);
      }
    }

    m_dirty = false;
  }

private:
  mutable std::string       m_cached;
  mutable bool              m_dirty = false;
  mutable bool              m_has_root = false;
  std::vector<std::string>  m_parts;
};

inline bool exists(const fs::path& p) {
  struct stat info = {};
  return 0 == stat(p.c_str(), &info);
}

inline bool is_file(const fs::path& p) {
  struct stat info = {};
  if (0 != stat(p.c_str(), &info)) { return false;}
  return S_IFREG == (info.st_mode & S_IFREG);
}

inline bool is_directory(const fs::path& p) {
  struct stat info = {};
  if (0 != stat(p.c_str(), &info)) { return false;}
  return S_IFDIR == (info.st_mode & S_IFDIR);
}

} // namespace fs
} // namespace tr

#endif // TINY_RENDERR_FS_PATH_H