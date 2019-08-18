/*
base256

Copyright (c) 2019, Lode Vandevenne
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// clang++ -std=c++11 base256.cpp -O3 -o base256

bool load_file(const std::string& filename, std::string* result) {
  std::ifstream file(filename.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
  if(!file) return false;

  std::streamsize size = 0;
  if(file.seekg(0, std::ios::end).good()) size = file.tellg();
  if(file.seekg(0, std::ios::beg).good()) size -= file.tellg();

  result->resize(size_t(size));
  if(size > 0) file.read((char*)(&(*result)[0]), size);

  return true;
}

void save_file(const std::string& buffer, const std::string& filename) {
  std::ofstream file(filename.c_str(), std::ios::out|std::ios::binary);
  file.write(buffer.empty() ? 0 : (char*)&buffer[0], std::streamsize(buffer.size()));
}

template<typename T>
std::string valtostr(const T& val, bool hex = false) {
  std::ostringstream sstream;
  if(hex) sstream << std::hex;
  sstream << val;
  return sstream.str();
}

template<typename T>
T strtoval(const std::string& s) {
  std::istringstream sstream(s);
  T val;
  sstream >> val;
  return val;
}

////////////////////////////////////////////////////////////////////////////////

static const int REPL_CHAR = 0xFFFD; // replacement character

std::vector<int> utf8_to_unicode(const std::vector<uint8_t>& a) {
  std::vector<int> result;
  for(int i = 0; i < a.size(); i++) {
    int b0 = a[i];
    int b1 = i + 1 < a.size() ? a[i + 1] : 255;
    int b2 = i + 2 < a.size() ? a[i + 2] : 255;
    int b3 = i + 3 < a.size() ? a[i + 3] : 255;

    if(b0 < 128) {
      result.push_back(b0);
    } else if(b0 < 194) {
      result.push_back(REPL_CHAR);
    } else if(b0 < 224) {
      if((b1 & 192) != 128) { result.push_back(REPL_CHAR); continue; }
      i++;
      result.push_back(((b0 & 31) << 6) + (b1 & 63));
    } else if(b0 < 240) {
      if((b1 & 192) != 128) { result.push_back(REPL_CHAR); continue; }
      if(b0 == 224 && b1 < 160) { result.push_back(REPL_CHAR); continue; };
      i++;
      if((b2 & 192) != 128) { result.push_back(REPL_CHAR); continue; }
      i++;
      result.push_back(((b0 & 15) << 12) + ((b1 & 63) << 6) + (b2 & 63));
    } else if(b0 < 245) {
      if((b1 & 192) != 128) { result.push_back(REPL_CHAR); continue; }
      if(b0 == 240 && b1 < 144) { result.push_back(REPL_CHAR); continue; };
      if(b0 == 244 && b1 >= 144) { result.push_back(REPL_CHAR); continue; };
      i++;
      if((b2 & 192) != 128) { result.push_back(REPL_CHAR); continue; }
      i++;
      if((b3 & 192) != 128) { result.push_back(REPL_CHAR); continue; }
      i++;
      result.push_back(((b0 & 7) << 18) + ((b1 & 63) << 12) + ((b2 & 63) << 6) + (b3 & 63));
    } else {
      result.push_back(REPL_CHAR);
    }
  }
  return result;
}

std::vector<uint8_t> unicode_to_utf8(const std::vector<int>& a) {
  std::vector<uint8_t> result;
  for(int i = 0; i < a.size(); i++) {
    int code_point = a[i];
    if(code_point < 128) {
      result.push_back(code_point);
    } else if(code_point <= 2047) {
      result.push_back((code_point >> 6) + 192);
      result.push_back((code_point & 63) + 128);
    } else if(code_point <= 65535) {
      result.push_back((code_point >> 12) + 224);
      result.push_back(((code_point >> 6) & 63) + 128);
      result.push_back((code_point & 63) + 128);
    } else if(code_point <= 1114111) {
      result.push_back((code_point >> 18) + 240);
      result.push_back(((code_point >> 12) & 63) + 128);
      result.push_back(((code_point >> 6) & 63) + 128);
      result.push_back((code_point & 63) + 128);
    } else {
      // error, add replacement character
      result.push_back(239);
      result.push_back(191);
      result.push_back(189);
    }
  }
  return result;
}

std::vector<uint8_t> string_to_utf8(const std::string& s) {
  std::vector<uint8_t> result;
  for(int i = 0; i < s.size(); i++) {
    result.push_back((unsigned char)(s[i]));
  }
  return result;
}

std::string utf8_to_string(const std::vector<uint8_t>& a) {
  std::string result;
  for(int i = 0; i < a.size(); i++) {
    result.push_back(a[i]);
  }
  return result;
}

std::vector<int> string_to_unicode(const std::string& s) {
  return utf8_to_unicode(string_to_utf8(s));
}

std::string unicode_to_string(const std::vector<int>& a) {
  return utf8_to_string(unicode_to_utf8(a));
}

////////////////////////////////////////////////////////////////////////////////

struct UnixArgs {
  std::string command; // the whole command
  std::string binary; // the executable path
  std::string post; // values after an empty "--"

  struct Arg {
    std::vector<std::string> helpargs;
    std::string help;
    std::string value;
    std::string def;
    bool present;
  };

  std::vector<Arg> args;
  std::map<char, size_t> chars;
  std::map<std::string, size_t> strings;

  std::vector<std::string> loose;

  std::string error;

  bool allowpost = false;
  bool allowloose = true;

  bool present(char c) const {
    return chars.count(c) && args[chars.find(c)->second].present;
  }

  bool present(const std::string& s) const {
    return strings.count(s) && args[strings.find(s)->second].present;
  }

  std::string value(const std::string& s) const {
    return strings.count(s) ? args[strings.find(s)->second].value : "";
  }

  void registerArg(char c, const std::string& s, const std::string& help = "", const std::string& def = "") {
    size_t index = args.size();
    args.resize(args.size() + 1);
    if(!s.empty()) {
      strings[s] = index;
      args.back().helpargs.push_back("--" + s);
      if(!def.empty()) args.back().helpargs.back() += "=[default:" + def + "]";
    }
    if(c) {
      chars[c] = index;
      args.back().helpargs.push_back("-" + std::string(1, c));
    }
    args.back().help = help;
    args.back().value = def;
    args.back().def = def;
  }

  bool parse(int argc, char *argv[]) {
    if(!argc) {
      error = "zero length arguments";
      return false;
    }

    binary = argv[0];

    for(int i = 0; i < argc; i++) {
      if(i > 0) command += " ";
      command += std::string(argv[i]);
    }

    for(int i = 1; i < argc; i++) {
      int num = 0;
      std::string s = argv[i];
      if(s == "--") {
        if(!allowpost) {
          error = "invalid loose double dash argument";
          return false;
        }
        for(int j = i + 1; j < argc; j++) {
          if(j > i + 1) post += " ";
          post += std::string(argv[j]);
        }
        break;
      } else if(s.size() > 2 && s[0] == '-' && s[1] == '-') {
        std::string s2 = s.substr(2);
        std::string val = "";
        size_t eq = s2.find('=');
        if(eq != std::string::npos) {
          val = s2.substr(eq + 1);
          s2 = s2.substr(0, eq);
        }
        if(!strings.count(s2)) {
          error = "unkonwn argument: " + s2;
          return false;
        }
        Arg& arg = args[strings[s2]];
        arg.present = true;
        if(!val.empty()) {
          arg.value = val;
        }
      } else if(s.size() > 1 && s[0] == '-') {
        for(int j = 1; j < s.size(); j++) {
          char c = s[j];
          if(!chars.count(c)) {
            error = "unkonwn argument: " + std::string(1, c);
            return false;
          }
          args[chars[c]].present = true;
        }
      } else {
        if(s == "-") {
          error = "loose single dash argument invalid";
          return false;
        }
        if(!allowloose) {
          error = "loose argument " + s + " invalid";
          return false;
        }
        loose.push_back(s);
      }
    }
    return true;
  }

  void printHelp(size_t indent) const {
    std::string ind;
    for(size_t i = 0; i < indent; i++) ind += " ";
    for(size_t i = 0; i < args.size(); i++) {
      const Arg& arg = args[i];
      std::cout << ind;
      for(size_t j = 0; j < arg.helpargs.size(); j++) {
        if(j > 0) std::cout << ", ";
        std::cout << arg.helpargs[j];
      }
      if(!arg.help.empty()) {
        std::cout << ": " << arg.help;
      }
      std::cout << std::endl;
    }
  }
};


////////////////////////////////////////////////////////////////////////////////

static const int U_NULL = 0x2400; // unicode Control Picture NUL.
static const int U_DEL = 0x2421; // unicode Control Picture DEL.
static const int U_SP = 0x2420; // unicode Control Picture SP

static const int ALT_NULL = 0x2205; // empty set symbol indicating null
static const int ALT_SP = 0x2423; // open box indicating space
static const int ALT_SHY = 0x2E1A; // hyphen with dots indicating soft hyphen SHY (it does quite look like a hyphen with a shy face)
static const int ALT_NBSP = 0x25AF; // a non-filled square indicating NBSP
static const int ALT_DEL = 0x2302; // "house" symbol for DEL

int table437[256] = {
  ALT_NULL, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
  0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
  0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
  0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC,
  ALT_SP, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
  0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
  0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
  0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
  0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
  0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
  0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
  0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
  0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2302,
  0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
  0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
  0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
  0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
  0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
  0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
  0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
  0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
  0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
  0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
  0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
  0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
  0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
  0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
  0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
  0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, ALT_NBSP
};

int ALT_81 = 0x201B; // normally 'C1 control code HOP'
int ALT_8D = 0x010C; // normally 'C1 control code RI'
int ALT_8F = 0x017F; // normally 'C1 control code SS3'
int ALT_90 = 0x0111; // normally 'C1 control code DCS'
int ALT_9D = 0x010D; // normally 'C1 control code OSC'

// Code page 1252, but missing spots filled up with other characters to have
// the full 256 unique glyphs: greek characters in rows 0/1 and characters that
// blend in for rows 8 and 9.
int table1252[256] = {
  ALT_NULL, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7,
  0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03B0,
  0x03C0, 0x03C1, 0x03C2, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7,
  0x03C8, 0x03C9, 0x03CA, 0x03CB, 0x03AC, 0x03CD, 0x03CE, 0x03AF,
  ALT_SP, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
  0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
  0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
  0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
  0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
  0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
  0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
  0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
  0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E,  ALT_DEL,
  0x20AC, ALT_81, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
  0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, ALT_8D, 0x017D, ALT_8F,
  ALT_90, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, ALT_9D, 0x017E, 0x0178,
  ALT_NBSP, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, ALT_SHY, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
  0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
  0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
  0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
  0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
  0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF
};

std::map<int, int> invertTable(int table[256]) {
  std::map<int, int> result;
  for(int i = 0; i < 256; i++) result[table[i]] = i;
  if(result.size() != 256) std::cout << "Warning: duplicates found" << std::endl;
  return result;
}

std::map<int, int> inv437 = invertTable(table437);
std::map<int, int> inv1252 = invertTable(table1252);

class Format {
 public:
  virtual ~Format() {}
  // prev and next char not used by all formats, and set to 0 if nonexistant
  virtual std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) = 0;

  virtual std::string decode(const std::string& s) {
    std::cerr << "Decode not implemented for this format!" << std::endl;
    return "";
  }

  // some don't support streaming due to using open and close
  // TODO: redesign the Format API to always stream for both encode and
  //       decode and have the Printer use those in a streaming manner
  virtual bool supportsStreaming() const {
    return true;
  }

  // size (in glyphs) of each printed byte
  virtual int width() const {
    return 1;
  }

  // whether there should be a space between the values (this adds one more
  // to width in addition to width())
  virtual bool space() const {
    return false;
  }

  // whether this format prints most printable ascii characters as-is (small
  // exceptions, such as escape codes for string literals, may happen).
  // if this is true, then no separate "mix" mode that mixes in printable
  // ascii characters in the notation will be supported for this format (since
  // it already prints printable ascii characters on its own)
  virtual bool printable() const {
    return false;
  }

  // could be e.g. q quote character for C string literal
  virtual std::string open() {
    return "";
  }

  // could be e.g. q quote character for C string literal
  virtual std::string close() {
    return "";
  }

  // optional extra to begin in-between lines with, e.g. " for C string literal
  virtual std::string linebeg() {
    return "";
  }

  // optional extra to end in-between lines with, e.g. " for C string literal
  virtual std::string lineend() {
    return "";
  }

  // whether the string representation allows line wrapping at all
  virtual bool allowlinebreaks() {
    return true;
  }

  // align based on input byte size or output character size
  virtual bool outwidth() {
    return false;
  }
};

class Printer {
 public:
  Printer(Format* n) : n(n) {}

  // width = original width before adding formatting like ANSI colors
  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next, size_t* width) {
    std::string temp = n->encodeChar(c, prev, next);
    *width = temp.size();
    if(printspace && c == 32 && (mix || n->printable())) {
      std::string result;
      for(size_t i = 0; i < n->width(); i++) {
        result += " ";
      }
      return result;
    }
    if(temp == "\n") return temp;
    if(c == 10 && printnewline && (mix || n->printable())) return "\n";
    std::string result;
    if(mix && c > 32 && c < 127 && !n->printable()) {
      size_t size = n->width() - 1;
      for(size_t i = 0; i < size; i++) {
        result += " ";
      }
      result += c;
      return result;
    }
    bool usecolor = colored && !(temp.size() == 1 && (unsigned char)temp[0] == c);
    if(usecolor) {
      // the gray values (7, 8, 15) are unused in case standard color for printable ASCII is used
      static const int colors[16] = {1,  2,  3, 7, 15, 8, 15,  4,
                                     9, 10, 11, 5, 13, 6, 14, 12};
      int color = colors[c >> 4];
      result += std::string() + "\x1b" + "[" + valtostr((color < 8 ? 30 : 82) + color) + "m";
      // dark blue is quite unreadable against black background, so give it a background. Also
      // give background to the one duplicate color to distinguish it.
      // This assumes terminal with white-ish text on black background
      if(color == 4 || (c >> 4) == 6) result += std::string() + "\x1b" + "[" + valtostr(100) + "m";
      if((c >> 4) == 6) result += std::string() + "\x1b" + "[" + valtostr(100) + "m";
      result += temp;
      result += "\x1b[0m";
      if(color == 4 || (c >> 4) == 6) result += "\x1b[0m";
    } else {
      result += temp;
    }
    return result;
  }

  std::string encode(const std::string& s) {
    size_t lnlen = valtostr(s.size(), linenumbersbase == 16).size();
    std::string result;
    for(int i = 0; i < s.size(); i++) {
      bool wrapped = false;
      if(wrap && numbytes >= wrap) {
        result += n->lineend();
        if(n->allowlinebreaks()) result += "\n";
        numbytes = 0;
        wrapped = true;
      }
      if(printlinenumbers && numbytes == 0) {
        std::string ln = valtostr(i, linenumbersbase == 16);
        while (ln.size() < lnlen) ln = " " + ln;
        result += ln + ": ";
      }
      if(i == 0) result += n->open();
      if(wrapped) result += n->linebeg();
      unsigned char prev = (i > 0) ? s[i - 1] : 0;
      unsigned char next = (i + 1 < s.size()) ? s[i + 1] : 0;
      size_t outwidth = 0;
      std::string temp = encodeChar(s[i], prev, next, &outwidth);
      result += temp;
      if(comma) result += ",";
      if(comma || n->space()) result += " ";
      if(temp == "\n") {
        numbytes = 0;
      }
      if(n->outwidth()) {
        // For C string literals, with variable length characters, align the width
        // based on the output width rather than the input width.
        numbytes += outwidth;
      } else {
        numbytes++;
      }
    }
    result += n->close();
    return result;
  }

  std::string decode(const std::string& s) {
    return n->decode(s);
  }

  std::string getTable() {
    std::string digits = "0123456789abcdef";
    int size = n->width() + 1;
    std::string table = "   ";
    for(int i = 0; i < 16; i++) {
      table += digits[i];
      for(int j = 1; j < size; j++) table += ' ';
    }
    table += '\n';
    table += " +";
    for(int i = 0; i < 16 * size; i++) {
      table += '-';
    }
    table += '\n';
    for(int y = 0; y < 16; y++) {
      table += digits.substr(y, 1) + "|";
      for(int x = 0; x < 16; x++) {
        unsigned char c = y * 16 + x;
        size_t outwidth = 0;
        std::string s = encodeChar(c, 0, 0, &outwidth);
        if(s == "\n") {
          s = "";
          for(size_t j = 1; j < size; j++) s += " ";
        }
        while(s.size() < n->width()) s = " " + s;
        table += std::string(" ") + s;
      }
      table += "\n";
    }
    return table;
  }

  Format* n;

  int wrap = 0;
  int numbytes = 0; // for wrap
  bool printlinenumbers = false;
  int linenumbersbase = 10;
  bool colored = false;
  bool comma = false;
  bool mix = false;
  bool printnewline = false;
  bool printspace = false;
};

class CP437 : public Format {
 public:
  CP437(bool newline, bool printnull) : newline(newline), printnull(printnull) {
  }

  virtual bool printable() const { return true; }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    if(newline && c == 10) return "\n";
    if(printnull && c == 0) return " ";
    return unicode_to_string({table437[c]});
  }

  std::string decode(const std::string& s) {
    if(newline) return "not supported with printnewline enabled";
    if(printnull) return "not supported with printnull enabled";
    std::vector<int> u = string_to_unicode(s);
    std::string result;
    for(int i = 0; i < u.size(); i++) {
      if(u[i] == 10) {
        continue;
      } else if(!inv437.count(u[i])) {
        std::cout << "invalid character: " << u[i] << " at " << i << std::endl;
        result.push_back('?');
      } else {
        result.push_back(inv437[u[i]]);
      }
    }
    return result;
  }

  bool newline;
  bool printnull;
};

class CP1252 : public Format {
 public:
  CP1252(bool newline, bool printnull) : newline(newline), printnull(printnull) {
  }

  virtual bool printable() const { return true; }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    if(newline && c == 10) return "\n";
    if(printnull && c == 0) return " ";
    return unicode_to_string({table1252[c]});
  }

  std::string decode(const std::string& s) {
    if(newline) return "not supported with printnewline enabled";
    if(printnull) return "not supported with printnull enabled";
    std::vector<int> u = string_to_unicode(s);
    std::string result;
    for(int i = 0; i < u.size(); i++) {
      if(u[i] == 10) {
        continue;
      } else if(!inv1252.count(u[i])) {
        std::cout << "invalid character: " << u[i] << " at " << i << std::endl;
        result.push_back('?');
      } else {
        result.push_back(inv1252[u[i]]);
      }
    }
    return result;
  }

  bool newline;
  bool printnull;
};

class Braille : public Format {
 public:
  Braille(bool newline, bool printnull) : newline(newline), printnull(printnull) {
  }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    if(c == 0 && !printnull) return unicode_to_string({table437[0]});
    if(newline && c == 10) return "\n";
    return unicode_to_string({0x2800 + c});
  }

  bool newline;
  bool printnull;
};

class ASCII : public Format {
 public:
  ASCII(bool newline = false) : newline(newline) {
  }

  virtual bool printable() const { return true; }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    if(newline && c == 10) return "\n";
    if(c > 32 && c < 127) return std::string(1, (char)c);
    else return "?";
  }

  bool newline;
};

class Low : public Format {
 public:
  Low(bool newline = false, bool lower = false) : newline(newline),
      digits(lower ? digits_lower : digits_upper)  {
  }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    return std::string("") + digits[c & 15];
  }

  const char* digits_lower = "0123456789abcdef";
  const char* digits_upper = "0123456789ABCDEF";
  const char* digits;

  bool newline;
};

class High : public Format {
 public:
  High(bool newline = false, bool lower = false) : newline(newline),
      digits(lower ? digits_lower : digits_upper)  {
  }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    return std::string("") + digits[(c >> 4) & 15];
  }

  const char* digits_lower = "0123456789abcdef";
  const char* digits_upper = "0123456789ABCDEF";
  const char* digits;

  bool newline;
};

class Base64 : public Format {
 public:
  Base64() {}

  virtual bool supportsStreaming() const {
    return false;
  }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    int r = num % 3;
    num++;
    if(r == 0) v = (c << 16u);
    else if(r == 1) v |= (c << 8u);
    else v |= c;
    std::string result;
    if(r == 0) {
      result += BASE64[(v >> 18) & 0x3f];
    } else if(r == 1) {
      result += BASE64[(v >> 12) & 0x3f];
    } else {
      result += BASE64[(v >>  6) & 0x3f];
      result += BASE64[(v >>  0) & 0x3f];
    }
    return result;
  }

  virtual std::string close() {
    int r = num % 3;
    std::string result;
    if(r == 0) {
    }
    if(r == 1) {
      result += BASE64[(v >> 12) & 0x3f];
      result += "==";
    }
    if(r == 2) {
      result += BASE64[(v >> 6) & 0x3f];
      result += "=";
    }
    return result;
  }

  int fromBase64(int v) {
    if(v >= 'A' && v <= 'Z') return (v - 'A');
    if(v >= 'a' && v <= 'z') return (v - 'a' + 26);
    if(v >= '0' && v <= '9') return (v - '0' + 52);
    if(v == '+') return 62;
    if(v == '/') return 63;
    return 0; //v == '='
  }


  std::string decode(const std::string& s) {
    std::string result;
    for(size_t i = 0; i + 3 < s.size(); i += 4) {
      int v = 262144 * fromBase64(s[i]) + 4096 * fromBase64(s[i + 1]) + 64 * fromBase64(s[i + 2]) + fromBase64(s[i + 3]);
      result += ((v >> 16) & 0xff);
      if(s[i + 2] != '=') result += ((v >> 8) & 0xff);
      if(s[i + 3] != '=') result += ((v >> 0) & 0xff);
    }
    return result;
  }

  virtual int width() const {
    return 1;
  }

  const char* BASE64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  size_t v = 0;
  size_t num = 0;
};

class Hex : public Format {
 public:
  Hex(bool prefix = false, bool lower = false) : prefix(prefix),
      digits(lower ? digits_lower : digits_upper)  {
  }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    std::string result;
    result += digits[(c >> 4) & 15];
    result += digits[c & 15];
    if(prefix) result = "0x" + result;
    return result;
  }

  std::string decode(const std::string& s) {
    std::string result;
    int count = 0;
    int val = 0;
    for(size_t i = 0; i < s.size(); i++) {
      if(prefix && i + 2 < s.size() && s[i] == '0' && s[i + 1] == 'x') i += 2;
      char c = s[i];
      if((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
        int d = 0;
        if(c >= '0' && c <= '9') d = c - '0';
        if(c >= 'a' && c <= 'f') d = c - 'a' + 10;
        if(c >= 'A' && c <= 'F') d = c - 'A' + 10;
        if(count % 2 == 1) {
          val <<= 4;
          val |= d;
          result.push_back(val);
        } else {
          val = d;
        }
        count++;
      }
    }
    return result;
  }

  virtual int width() const {
    return prefix ? 4 : 2;
  }

  virtual bool space() const {
    return true;
  }

  const char* digits_lower = "0123456789abcdef";
  const char* digits_upper = "0123456789ABCDEF";
  const char* digits;

  bool prefix;
};

class Decimal : public Format {
 public:
  Decimal(bool prefix = false) : prefix(prefix) {
  }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    static const std::string d = "0123456789";
    std::string result;
    if(prefix) {
      if(c >= 100) result += d[c / 100];
      if(c >= 10) result += d[(c / 10) % 10];
      result += d[c % 10];
    } else {
      result += d[c / 100];
      result += d[(c / 10) % 10];
      result += d[c % 10];
    }
    return result;
  }

  std::string decode(const std::string& s) {
    std::string result;
    int count = 0;
    int val = 0;
    for(size_t i = 0; i < s.size(); i++) {
      char c = s[i];
      if((c >= '0' && c <= '9')) {
        int d = c - '0';
        if(count % 3 == 0) {
          val = d;
        } else {
          val *= 10;
          val += d;
        }
        if(count % 3 == 2) result.push_back(val);
        count++;
      }
    }
    return result;
  }

  virtual int width() const {
    return 3;
  }

  virtual bool space() const {
    return true;
  }

  bool prefix;
};

class Octal : public Format {
 public:
  Octal(bool prefix = false) : prefix(prefix) {
  }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    static const std::string d = "01234567";
    std::string result;
    result += d[c / 64];
    result += d[(c / 8) % 8];
    result += d[c % 8];
    if(prefix) result = "0" + result;
    return result;
  }

  virtual int width() const {
    return prefix ? 4 : 3;
  }

  virtual bool space() const {
    return true;
  }

  bool prefix;
};

class Binary : public Format {
 public:
  Binary(bool prefix = false) : prefix(prefix) {
  }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    std::string result;
    for(int j = 0; j < 8; j++) {
      result += ((c >> (7 - j)) & 1) ? '1' : '0';
    }
    if(prefix) result = "0b" + result;
    return result;
  }

  virtual int width() const {
    return prefix ? 10 : 8;
  }

  virtual bool space() const {
    return true;
  }

  bool prefix;
};



class Colored : public Format {
 public:
  Colored() {
  }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    std::string result;
    result = std::string() + "\x1b" + "[48;5;" + valtostr((int)c) + "m" + " " + "\x1b[0m";
    return result;
  }
};

// strict ANSI-C compatible string
class CString : public Format {
 public:
  CString() {}

  virtual bool printable() const { return true; }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    if(c == '\a') return "\\a";
    if(c == '\b') return "\\b";
    if(c == '\f') return "\\f";
    if(c == '\n') return "\\n";
    if(c == '\r') return "\\r";
    if(c == '\t') return "\\t";
    if(c == '\v') return "\\v";
    if(c == '\\') return "\\\\";
    if(c == '"') return "\\\"";
    if(c == '?' && prev == '?') return "\\?"; // prevent trigraphs
    if(c >= 32 && c < 127) {
      std::string result;
      result += c;
      return result;
    }
    // octal
    // octal is used, not hexadecimal ("\x..."), because octal is defined to end after 3 characters, while
    // hexadecimal can keep going on indefinitely (it doesn't end after 2) as long as valid hex digits follow,
    // so hexadecimal is harder to deal with if an actual 0-7 or a-f character would follow (requires ending and reopening the string literal)
    bool digitnext = (next >= '0' && next <= '7');
    int o0 = (c >> 6) & 7;
    int o1 = (c >> 3) & 7;
    int o2 = (c >> 0) & 7;
    std::string result = "\\";
    // shorten if possible, e.g. many nulls in a row will be \0\0\0\0 instead of \000\\000\000\000
    // using \0 and other less than 3 digit codes is only ok if no real octal digit after it, else the next character becomes part of the code
    if(c < 8 && !digitnext) {
      result += ('0' + o2);
    } else if (c < 64 && !digitnext) {
      result += ('0' + o1);
      result += ('0' + o2);
    } else {
      result += ('0' + o0);
      result += ('0' + o1);
      result += ('0' + o2);
    }
    return result;
  }

  virtual bool supportsStreaming() const {
    return false;
  }

  virtual std::string open() {
    return "\"";
  }

  virtual std::string close() {
    return "\"";
  }

  virtual std::string linebeg() {
    return "\"";
  }

  virtual std::string lineend() {
    return "\"";
  }

  virtual bool outwidth() {
    return true;
  }
};

// Java-compatible string, per byte, it does *not* group 2 bytes for UTF-16.
class JavaString : public Format {
 public:
  JavaString() {}

  virtual bool printable() const { return true; }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    if(c == '\b') return "\\b";
    if(c == '\f') return "\\f";
    if(c == '\n') return "\\n";
    if(c == '\r') return "\\r";
    if(c == '\t') return "\\t";
    if(c == '\\') return "\\\\";
    if(c == '"') return "\\\"";
    if(c >= 32 && c < 127) {
      std::string result;
      result += c;
      return result;
    }
    // octal: we work byte-based, so don't use \u unicode escapes, which are longer
    bool digitnext = (next >= '0' && next <= '7');
    int o0 = (c >> 6) & 7;
    int o1 = (c >> 3) & 7;
    int o2 = (c >> 0) & 7;
    std::string result = "\\";
    // shorten if possible, e.g. many nulls in a row will be \0\0\0\0 instead of \000\\000\000\000
    // using \0 and other less than 3 digit codes is only ok if no real octal digit after it, else the next character becomes part of the code
    if(c < 8 && !digitnext) {
      result += ('0' + o2);
    } else if (c < 64 && !digitnext) {
      result += ('0' + o1);
      result += ('0' + o2);
    } else {
      result += ('0' + o0);
      result += ('0' + o1);
      result += ('0' + o2);
    }
    return result;
  }

  virtual bool supportsStreaming() const {
    return false;
  }

  virtual std::string open() {
    return "\"";
  }

  virtual std::string close() {
    return "\"";
  }

  virtual std::string linebeg() {
    return "\"";
  }

  virtual std::string lineend() {
    return "\" +";
  }

  virtual bool outwidth() {
    return true;
  }
};

// JS-compatible string, per byte, it does *not* group 2 bytes for UTF-16.
class JSString : public Format {
 public:
  JSString() {}

  virtual bool printable() const { return true; }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    if(c == '\b') return "\\b";
    if(c == '\f') return "\\f";
    if(c == '\n') return "\\n";
    if(c == '\r') return "\\r";
    if(c == '\t') return "\\t";
    if(c == '\\') return "\\\\";
    if(c == '\'') return "\\'";
    if(c >= 32 && c < 127) {
      std::string result;
      result += c;
      return result;
    }
    // in JS \0 is different than other octal, and should not have any *decimal* digit after it
    bool digitnext = (next >= '0' && next <= '9');
    if(c == 0 && !digitnext) return "\\0";
    int h0 = (c >> 4) & 15;
    int h1 = (c >> 0) & 15;
    static const std::string hex = "0123456789ABCDEF";
    std::string result = "\\x";
    result += hex[h0];
    result += hex[h1];
    return result;
  }

  virtual bool supportsStreaming() const {
    return false;
  }

  virtual std::string open() {
    return "'";
  }

  virtual std::string close() {
    return "'";
  }

  virtual std::string linebeg() {
    return "'";
  }

  virtual std::string lineend() {
    return "' +";
  }

  virtual bool outwidth() {
    return true;
  }
};

// Different than JS string: using double quotes, and can only escape with \u, no \x or octal
class JSONString : public Format {
 public:
  JSONString() {}

  virtual bool printable() const { return true; }

  std::string encodeChar(unsigned char c, unsigned char prev, unsigned char next) {
    if(c == '\b') return "\\b";
    if(c == '\f') return "\\f";
    if(c == '\n') return "\\n";
    if(c == '\r') return "\\r";
    if(c == '\t') return "\\t";
    if(c == '\\') return "\\\\";
    if(c == '\"') return "\\\"";
    if(c >= 32 && c < 127) {
      std::string result;
      result += c;
      return result;
    }
    static const std::string hex = "0123456789ABCDEF";
    int h0 = (c >> 4) & 15;
    int h1 = (c >> 0) & 15;
    std::string result = "\\u00";
    result += hex[h0];
    result += hex[h1];
    return result;
  }

  virtual bool supportsStreaming() const {
    return false;
  }

  virtual std::string open() {
    return "\"";
  }

  virtual std::string close() {
    return "\"";
  }

  // Cannot break up string into multiple lines in JSON
  virtual bool allowlinebreaks() {
    return false;
  }

  virtual bool outwidth() {
    return true;
  }
};

////////////////////////////////////////////////////////////////////////////////

void printHelp(const UnixArgs& args) {
  if(!args.error.empty()) {
    std::cout << "ERROR: " << args.error << std::endl << std::endl;
  }

  std::cout << "base256, a base-256 viewer (as opposed to a base-16 hexdump).\n"
               "Converts binary data from bytes to various formats, by default using 256 unique unicode glyphs.\n"
               "Other formats, including hex, are supported as well.\n"
               "\n"
               "If no output file is given, outputs to stdout. If no input file is given either, inputs from stdin (e.g. for piping, e.g. when giving no arguments at all).\n"
               "\n"
               "Requires terminal unicode suppport and/or ANSI color support, depending on the format and flags\n"
               "Most encodings use unique symbols for each possible byte, but some print options may affect that property.\n"
               "Made by Lode Vandevenne\n"
               "\n";

  std::cout << "Usage:" << std::endl;
  std::cout << args.binary << " [-options] [in.bin] [--outfile=out.txt]" << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  args.printHelp(2);
}

int main(int argc, char *argv[]) {
  UnixArgs args;
  args.registerArg('h', "help", "show this help");
  args.registerArg('H', "table", "show a reference of characters for currently selected format");
  args.registerArg(0, "tables", "show tables of all existing formats (printed result depends on modifications like --color)");
  args.registerArg(0, "outfile", "write to given output file instead of printing in terminal");
  args.registerArg('d', "decode", "decode back from the format to binary data. Only works for some formats, and only without line numbers and without printnewline.");
  args.registerArg(0, "format", "Format to use. Use -H or --tables to view their tables. Formats:\n"
      "      cp437: 256 unique characters, based on code page 437 (with small modifications to make all unique and none empty)\n"
      "      cp1252: 256 unique characters, based on code page 1252 (with small modifications to make all unique and none empty)\n"
      "      braille: 256 unique characters, using Unicode Braille patterns (in Unicode order)\n"
      "      ascii: only print ASCII printable characters as themselves, others are shown as '?'\n"
      "      hex: print bytes in hexadecimal\n"
      "      dec: print bytes in decimal (3 digits, prepended with zeros, unless --prefix is enabled)\n"
      "      oct: print bytes in octal\n"
      "      bin: print bytes in binary\n"
      "      low: characters are shown as their least significant hex digit (use --mix to recover printable chars)\n"
      "      high: characters are shown as their most significant hex digit (use --mix to recover printable chars)\n"
      "      colored: Uses the 256 ANSI colors. Requires 256 background color support in terminal. Different than --color.\n"
      "      c: ANSI C string literal (note that C ends strings at the first \\0. Use --size to print length too.)\n"
      "      java: Java string literal (byte based, no UTF-16)\n"
      "      js: JS string literal (byte based, no UTF-16)\n"
      "      json: JSON string literal",
      "cp437");
  args.registerArg('m', "mix", "Override the format for printable ASCII characters with the printable ASCII characters themselves.");
  args.registerArg(0, "printnewline", "Print newlines as actual newline in modes 'cp437', 'cp1252' and 'ascii'.");
  args.registerArg(0, "printspace", "For any format that uses ASCII characters but doesn't print space as empty, print the space as empty space anyway.");
  args.registerArg(0, "printnull", "For formats where null is normally invisible but a 'empty set' symbol is printed, use invisible character anyway.");
  args.registerArg('x', "", "shortcut for --format=hex");
  args.registerArg('0', "", "shortcut for --format=dec");
  args.registerArg('1', "", "shortcut for --format=cp1252");
  args.registerArg('4', "", "shortcut for --format=cp437");
  args.registerArg('a', "", "shortcut for --format=ascii");
  args.registerArg('m', "", "shortcut for --format=hex --mix");
  args.registerArg(0, "prefix", "For number based formats, use C-style number prefixes."); // and avoids 0 in front of decimal numbers
  args.registerArg(0, "comma", "Add comma between characters (useful for numeric formats).");
  args.registerArg(0, "lower", "Use lower case instead of upper case for hex digits.");
  args.registerArg(0, "upper", "Use upper case hex digits (no need to give this flag, uppercase is the default).");
  args.registerArg('c', "color", "Use ANSI color codes (works in unix shell) for non-ASCII characters, based on high hex digit (works with all format, is independent from --format=colored).");
  args.registerArg('n', "", "no extra newline at end of output.");
  // TODO: use "wrap" for output bytes instead and call it "align" for input bytes
  args.registerArg(0, "wrap", "Add newlines every so many input bytes", "64");
  args.registerArg('w', "", "shortcut for --wrap=64");
  args.registerArg('W', "", "shortcut for --wrap=100");
  args.registerArg('l', "", "display line numbers (starting byte index), in decimal. Only useful with wrap or printnewline.");
  args.registerArg('L', "", "display line numbers (starting byte index), in hexadecimal. Only useful with wrap or printnewline.");
  args.registerArg('s', "size", "print size in bytes at the end");

  if(!args.parse(argc, argv) || args.present("help")) {
    printHelp(args);
    return 0;
  }

  std::string infile = args.loose.size() > 0 ? args.loose[0] : "";
  std::string outfile = args.present("outfile") ? args.value("outfile") : "";

  bool mix = args.present("mix");
  bool printspace = args.present("printspace");
  bool printnull = args.present("printnull");
  bool printnewline = args.present("printnewline");
  bool comma = args.present("comma");
  bool prefix = args.present("prefix");
  bool colored = args.present("color");
  bool lower = args.present("lower") || !args.present("upper");
  bool printlinenumbers = args.present('l') || args.present('L');
  int linenumbersbase = args.present('L') ? 16 : 10;
  bool printsize = args.present('s') || args.present("size");
  size_t size = 0;

  std::string formatname = args.value("format");
  if(args.present('x')) formatname = "hex";
  if(args.present('0')) formatname = "dec";
  if(args.present('1')) formatname = "cp1252";
  if(args.present('4')) formatname = "cp437";
  if(args.present('a')) formatname = "ascii";
  if(args.present('m')) { formatname = "hex"; mix = true; }

  int wrap = 0;
  if(args.present("wrap")) {
    wrap = 64;
  } else if(args.present('W')) {
    wrap = 100;
  } else if(args.present('w')) {
    wrap = 64;
  }
  if(args.present("wrap")) {
    wrap = strtoval<int>(args.value("wrap"));
  }

  std::vector<std::pair<std::string, Format*>> formats;
  formats.push_back({"cp437", new CP437(printnewline, printnull)});
  formats.push_back({"cp1252", new CP1252(printnewline, printnull)});
  formats.push_back({"braille", new Braille(printnewline, printnull)});
  formats.push_back({"ascii", new ASCII(printnewline)});
  formats.push_back({"base64", new Base64});
  formats.push_back({"hex", new Hex(prefix, lower)});
  formats.push_back({"dec", new Decimal(prefix)});
  formats.push_back({"oct", new Octal(prefix)});
  formats.push_back({"bin", new Binary(prefix)});
  formats.push_back({"low", new Low(printnewline, lower)});
  formats.push_back({"high", new High(printnewline, lower)});
  formats.push_back({"colored", new Colored()});
  formats.push_back({"c", new CString()});
  formats.push_back({"java", new JavaString()});
  formats.push_back({"js", new JSString()});
  formats.push_back({"json", new JSONString()});

  // The format that is itself colored is incompatible with the extra coloring
  if(colored && formatname == "colored") colored = false;

  Format* format = formats[0].second;
  if(formatname != "") {
    bool found = false;
    for(size_t i = 0; i < formats.size(); i++) {
      if(formatname == formats[i].first) {
        format = formats[i].second;
        found = true;
        break;
      }
    }
    if(!found) {
      std::cout << "unknown format: " << formatname << std::endl;
      return 1;
    }
  }

  if(args.present("tables")) {
    for(size_t i = 0; i < formats.size(); i++) {
      bool both = !formats[i].second->printable();
      for(size_t mix = 0; mix <= 1; mix++) {
        if(!both && mix == 1) continue;
        if(both) {
          std::cout << formats[i].first << " (" << (mix ? "with" : "without") << " --mix): " << std::endl;
        } else {
          std::cout << formats[i].first << ": " << std::endl;
        }

        Printer printer(formats[i].second);
        printer.colored = colored;
        printer.comma = comma;
        printer.mix = mix;
        printer.printspace = printspace;
        std::string table = printer.getTable();
        std::cout << table << std::endl;
      }
    }
    return 0;
  }

  Printer printer(format);

  printer.printlinenumbers = printlinenumbers;
  printer.linenumbersbase = linenumbersbase;
  printer.colored = colored;
  printer.comma = comma;
  printer.mix = mix;
  printer.printnewline = printnewline;
  printer.printspace = printspace;
  printer.wrap = wrap;

  if(args.present('H')) {
    std::cout << printer.getTable() << std::endl;
    return 0;
  }

  bool decode = args.present('d');

  // streaming
  if(infile.empty() && outfile.empty() && !decode && format->supportsStreaming()) {
    char c;
    std::string s = "a";
    while(std::cin.get(c)) {
      s[0] = c;
      std::cout << printer.encode(s);
      size++;
    }
    if(printsize) std::cout << std::endl << "size: " << size;
    if(!decode && !args.present('n')) std::cout << std::endl;
    return 0;
  }

  // non streaming, file based
  std::string file;
  if(!infile.empty()) {
    if(!load_file(infile, &file)) {
      std::cout << "invalid input file (use -h for help)" << std::endl;
      return 1;
    }
  } else {
    char c;
    while(std::cin.get(c)) {
      file.push_back(c);
    }
  }

  std::string result;

  if(decode) {
    result = printer.decode(file);
    size = result.size();
  } else {
    result = printer.encode(file);
    size = file.size();
  }

  if(outfile.empty()) {
    std::cout << result;
    if(printsize) std::cout << std::endl << "size: " << size;
    if(!decode && !args.present('n')) std::cout << std::endl;
  } else {
    if(printsize) result += "\nsize: " + valtostr(size);
    if(!decode && !args.present('n')) result += "\n";
    save_file(result, outfile);
  }
}
