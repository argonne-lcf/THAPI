#include "my_demangle.h"
#include <algorithm>
#include <climits>
#include <iomanip>
#include <iostream>
#include "json.hpp"

#include <regex>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

thapi_function_name f_demangle_name(thapi_function_name mangle_name) {
  std::string result = mangle_name;
  std::string line_num;  

  // C++ don't handle PCRE, hence and lazy/non-greedy and $.
  const static std::regex base_regex("__omp_offloading_[^_]+_[^_]+_(.*?)_([^_]+)$");
  std::smatch base_match;
  if (std::regex_match(mangle_name, base_match, base_regex) && base_match.size() == 3) {
    result = base_match[1].str();
    line_num = base_match[2].str();
  }

  const char *demangle = my_demangle(result.c_str());
  if (demangle) {
    thapi_function_name s{demangle};
    if (!line_num.empty())
       s += "_" + line_num;

    /* We name the kernels after the type that gets passed in the first
       template parameter to the sycl_kernel function in order to prevent
       it from conflicting with any actual function name.
       The result is the demangling will always be something like, “typeinfo for...”.
    */
    if (s.rfind("typeinfo name for ") == 0)
      return s.substr(18, s.size());
    return s;
  }
  return mangle_name;
};

template <typename T>
std::string to_string_with_precision2(const T a_value, const std::string extension,
                                      const int n = 2) {
  std::ostringstream out;
  out.precision(n);
  out << std::fixed << a_value << extension;
  return out.str();
}

class TallyCoreBase {
public:
  TallyCoreBase() {}

  TallyCoreBase(uint64_t _dur, bool _err) : duration{_dur}, error{_err} {
    count = 1;
    min = _dur;
    max = _dur;
  }

  uint64_t duration{0};
  bool error{0};
  uint64_t min{ULONG_MAX};
  uint64_t max{0};
  uint64_t count{0};
  double duration_ratio{1.};
  double average{0};

  // Pure virtual function is needed if not we have an
  // ` symbol lookup error: ./ici/lib/libXProf.so: undefined symbol: _ZTI13TallyCoreBase`
  virtual const std::vector<std::string> to_string() = 0;

  const auto to_string_size() {
    std::vector<long> v;
    for (auto &e : to_string())
      v.push_back(static_cast<long>(e.size()));
    return v;
  }

  TallyCoreBase &operator+=(const TallyCoreBase &rhs) {
    this->duration += rhs.duration;
    this->min = std::min(this->min, rhs.min);
    this->max = std::max(this->max, rhs.max);
    this->count += rhs.count;
    this->error += rhs.error;
    return *this;
  }

  void finalize(const TallyCoreBase &rhs) {
    average = count ? static_cast<double>(duration) / count : 0.;
    duration_ratio = static_cast<double>(duration) / rhs.duration;
  }

  bool operator>(const TallyCoreBase &rhs) { return duration > rhs.duration; }

  void update_max_size(std::vector<long> &m) {
    const auto current_size = to_string_size();
    for (auto i = 0U; i < current_size.size(); i++) {
      m[i] = std::max(m[i], current_size[i]);
    }
  }
};

class TallyCoreTime : public TallyCoreBase {
public:
  static constexpr std::array headers{"Time", "Time(%)", "Calls", "Average", "Min", "Max", "Error"};

  using TallyCoreBase::TallyCoreBase;
  virtual const std::vector<std::string> to_string() {
    return std::vector<std::string>{format_time2(duration),
                                    to_string_with_precision2(100. * duration_ratio, "%"),
                                    to_string_with_precision2(count, "", 0),
                                    format_time2(average),
                                    format_time2(min),
                                    format_time2(max),
                                    to_string_with_precision2(error, "", 0)};
  }

private:
  template <typename T> std::string format_time2(const T duration) {

    const double h = duration / 3.6e+12;
    if (h >= 1.)
      return to_string_with_precision2(h, "h");

    const double min = duration / 6e+10;
    if (min >= 1.)
      return to_string_with_precision2(min, "min");

    const double s = duration / 1e+9;
    if (s >= 1.)
      return to_string_with_precision2(s, "s");

    const double ms = duration / 1e+6;
    if (ms >= 1.)
      return to_string_with_precision2(ms, "ms");

    const double us = duration / 1e+3;
    if (us >= 1.)
      return to_string_with_precision2(us, "us");

    return to_string_with_precision2(duration, "ns");
  }
};

class TallyCoreByte : public TallyCoreBase {
public:
  static constexpr std::array headers{"Byte", "Byte(%)", "Calls", "Average", "Min", "Max", "Error"};

  using TallyCoreBase::TallyCoreBase;
  virtual const std::vector<std::string> to_string() {
    return std::vector<std::string>{format_byte2(duration),
                                    to_string_with_precision2(100. * duration_ratio, "%"),
                                    to_string_with_precision2(count, "", 0),
                                    format_byte2(average),
                                    format_byte2(min),
                                    format_byte2(max),
                                    to_string_with_precision2(error, "", 0)};
  }

private:
  template <typename T> std::string format_byte2(const T duration) {
    const double PB = duration / 1e+15;
    if (PB >= 1.)
      return to_string_with_precision2(PB, "PB");

    const double TB = duration / 1e+12;
    if (TB >= 1.)
      return to_string_with_precision2(TB, "TB");

    const double GB = duration / 1e+9;
    if (GB >= 1.)
      return to_string_with_precision2(GB, "GB");

    const double MB = duration / 1e+6;
    if (MB >= 1.)
      return to_string_with_precision2(MB, "MB");

    const double kB = duration / 1e+3;
    if (kB >= 1.)
      return to_string_with_precision2(kB, "kB");

    return to_string_with_precision2(duration, "B");
  }
};

//
//    /\   _   _  ._ _   _   _. _|_ o  _  ._
//   /--\ (_| (_| | (/_ (_| (_|  |_ | (_) | |
//         _|  _|        _|

/* Extract a subtutple
 * https://devblogs.microsoft.com/oldnewthing/20200623-00/?p=103901
 * Look like it may have some problem, but i was not smart enough
 * 1/ to understand the problem
 * 2/ To fix it
 */
template <class... Args, std::size_t... Is>
auto make_tuple_cuted(std::tuple<Args...> tp, std::index_sequence<Is...>) {
  return std::tuple{std::get<Is>(tp)...};
}

template <class... Args> auto make_tuple_cuted(std::tuple<Args...> tp) {
  return make_tuple_cuted(tp, std::make_index_sequence<sizeof...(Args) - 1>{});
}
/* Aggreate a map of <std::tuple<...>, any>  using the index of the tuple as the new key
 * this index is an `std::index_sequence`
 */
template <typename TC, class... T,
          typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
auto aggregate_by_name(std::unordered_map<std::tuple<T...>, TC> &m) {
  std::unordered_map<thapi_function_name, TC> aggregated{};
  for (auto const &[key, val] : m)
    aggregated[std::get<sizeof...(T) - 1>(key)] += val;
  return aggregated;
}

template <typename TC, class... T,
          typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
auto aggregate_nested(std::unordered_map<std::tuple<T...>, TC> &m) {
  // https://stackoverflow.com/a/42043006/7674852
  // REALY?!
  typedef decltype(make_tuple_cuted(std::declval<std::tuple<T...>>())) Minusone;
  std::unordered_map<Minusone, std::unordered_map<thapi_function_name, TC>> aggregated;
  for (auto &[key, val] : m) {
    auto head = make_tuple_cuted(key);
    aggregated[head][std::get<sizeof...(T) - 1>(key)] += val;
  }
  return aggregated;
}

// Add the elements of the tuple (t) in the set elements (s)
template <class... T, class... T2, size_t... I>
void add_to_set(std::tuple<T...> &s, std::tuple<T2...> t, std::index_sequence<I...>) {
  (std::get<I>(s).insert(std::get<I>(t)), ...);
}

template <class... T, size_t... I>
void remove_neutral(std::tuple<std::set<T>...> &s, std::index_sequence<I...>) {
  (std::get<I>(s).erase (T{}), ...);
}

// Loop over the map keys and return a tuple correspoding to the uniq elements
template <template <typename...> class Map, typename... K, typename V>
auto get_uniq_tally(Map<std::tuple<K...>, V> &input) {
  auto tuple_set = std::make_tuple(std::set<K>{}...);
  constexpr auto s = std::make_index_sequence<sizeof...(K)>();

  for (auto &m : input)
    add_to_set(tuple_set, m.first, s);

  remove_neutral(tuple_set, s);
  return tuple_set;
}

template <typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
void add_footer(std::vector<std::pair<thapi_function_name, TC>> &m) {

  // Create the final Tally
  TC tot{};
  for (auto const &keyval : m)
    tot += keyval.second;
  // Finalize (compute ratio)
  for (auto &keyval : m)
    keyval.second.finalize(tot);
  m.push_back(std::make_pair(std::string("Total"), tot));
}

//    __
//   (_   _  ._ _|_ o ._   _
//   __) (_) |   |_ | | | (_|
//                         _|
//

/* Take a map, and return a sorted vector of pair
 * sorted by original map values
 */

template <template <typename...> class Map, typename K, typename V>
auto sort_by_value(Map<K, V> &m) {
  std::vector<std::pair<K, V>> v;
  std::copy(m.begin(), m.end(), std::back_inserter<std::vector<std::pair<K, V>>>(v));
  std::sort(v.begin(), v.end(), [=](auto &a, auto &b) { return a.second > b.second; });
  return v;
}

inline std::string limit_string_size(std::string original, int u_size, std::string j = "[...]") {
  if ((u_size < 0) || ((uint)u_size >= original.length()))
    return original;

  if ((uint)u_size <= j.length())
    return j.substr(0, u_size);

  const unsigned size = u_size - j.length();
  const unsigned half = size / 2;
  const unsigned half_up = half + size % 2;

  const std::string prefix = original.substr(0, half);
  const std::string suffix = original.substr(original.length() - half_up, half_up);
  return prefix + j + suffix;
}

template <typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
void apply_sizelimit(std::vector<std::pair<thapi_function_name, TC>> &m, int max_name_size) {
  for (auto &kv : m)
    kv.first = limit_string_size(kv.first, max_name_size);
}

//                                  __                    __
//   |\/|  _.    o ._ _      ._    (_ _|_ ._ o ._   _    (_  o _   _
//   |  | (_| >< | | | | |_| | |   __) |_ |  | | | (_|   __) | /_ (/_
//                                                  _|
// TallyCoreHeader tuple of str
template <std::size_t SIZE, typename TC,
          typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
auto max_string_size2(std::vector<std::pair<thapi_function_name, TC>> &m,
                      const std::pair<std::string, std::array<const char *, SIZE>> header) {

  auto &[header_name, header_tallycore] = header;
  long name_max = header_name.size();

  // Know at compile time
  auto tallycore_max =
      std::apply([](auto &&...e) { return std::vector<long>{(static_cast<long>(strlen(e)))...}; },
                 header_tallycore);

  for (auto &[name, tallycore] : m) {
    name_max = std::max(static_cast<long>(name.size()), name_max);
    tallycore.update_max_size(tallycore_max);
  }
  // No need to display Error if none of them
  if (!m.back().second.error)
    tallycore_max.back() *= -1;

  return std::pair(name_max, tallycore_max);
}

//    __
//   /__    _|_  _  ._  |_   _  ._ o _   _. _|_ o  _  ._
//   \_| |_| |_ (/_ | | |_) (/_ |  | /_ (_|  |_ | (_) | |
//

// Our base datastructor is a pair of either tuple / TalyCore and a vector of int correspond to the
// collumn size. We print each tuple menber with the correct width, and joined with a `|`
// /!\ Ugly: If the collumn with is negative, that mean we should print a empty collumn of abs(size)
// This is usefull for the footer or for hiding the `error` collumn

// We use 3 function, because my template skill are poor...
template <std::size_t SIZE>
std::ostream &operator<<(std::ostream &os,
                         const std::pair<std::array<const char *, SIZE>, std::vector<long>> &_tup) {
  auto &[c, column_width] = _tup;
  for (auto i = 0U; i < c.size(); i++) {
    os << std::setw(std::abs(column_width[i]));
    if (column_width[i] <= 0)
      os << "" << "   ";
    else
      os << c[i] << " | ";
  }
  return os;
}

// Print the TallyCore
template <typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
std::ostream &operator<<(std::ostream &os, std::pair<TC, std::vector<long>> &_tup) {
  auto &[c, column_width] = _tup;
  const std::vector<std::string> v = c.to_string();
  for (auto i = 0U; i < v.size(); i++) {
    os << std::setw(std::abs(column_width[i]));
    if (column_width[i] <= 0)
      os << "" << "   ";
    else
      os << v[i] << " | ";
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, std::pair<std::string, long> &pair) {
  os << std::setw(pair.second) << pair.first << " | ";
  return os;
}

// Print 2 Tuple correspond to the hostname, process, ... device, subdevice.
template <class... T, class... T2, size_t... I>
void print_tally(std::ostream &os, const std::tuple<T...> &s, const std::tuple<T2...> &h,
                 std::index_sequence<I...>) {
  ((std::get<I>(s).size() ? os << std::get<I>(s).size() << " " << std::get<I>(h) << " | "
                            : os << ""),
   ...);
}

template <class... T, class... T2>
void print_tally(std::ostream &os, const std::string &header, const std::tuple<T...> &s,
                  const std::tuple<T2...> &h) {
  os << header << " | ";
  constexpr auto seq = std::make_index_sequence<sizeof...(T2)>();
  print_tally(os, s, h, seq);
  os << std::endl;
}

// Print 2 Tuple correspond to the hostname, process, ... device, subdevice.
template <class... T, class... T2, size_t... I>
void print_named_tuple(std::ostream &os, const std::tuple<T...> &s, const std::tuple<T2...> &h,
                       std::index_sequence<I...>) {
  (((std::get<I>(s) != T{}) ? os << std::get<I>(h) << ": " << std::get<I>(s) << " | "
                            : os << ""),
   ...);
}

template <class... T, class... T2>
void print_named_tuple(std::ostream &os, const std::string &header, const std::tuple<T...> &s,
                       const std::tuple<T2...> &h) {
  os << header << " | ";
  constexpr auto seq = std::make_index_sequence<sizeof...(T2)>();
  print_named_tuple(os, s, h, seq);
  os << std::endl;
}

// orignal_map is map where the key are tuple who correcpond to hostname, process, ..., API call
// name, and the value are TallyCore
template <typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
void print_tally2(std::unordered_map<thapi_function_name, TC> &m, int display_name_max_size) {

  auto sorted_by_value = sort_by_value(m);
  add_footer(sorted_by_value);
  apply_sizelimit(sorted_by_value, display_name_max_size);

  const auto header = std::make_pair(std::string("Name"), TC::headers);

  auto &&[s1, s2] = max_string_size2(sorted_by_value, header);

  auto f1_header = std::make_pair(header.first, s1);
  const auto f2_header = std::make_pair(header.second, s2);
  std::cout << f1_header << f2_header << std::endl;

  for (auto it = sorted_by_value.begin(); it != sorted_by_value.end(); ++it) {
    // For the total, hide the average, min and max
    if (std::next(it) == sorted_by_value.end()) {
      s2[3] *= -1;
      s2[4] *= -1;
      s2[5] *= -1;
    }
    const auto &[name, tallycore] = *it;
    auto f = std::make_pair(name, s1);
    auto f2 = std::make_pair(tallycore, s2);
    std::cout << f << f2 << std::endl;
  }
  std::cout << std::endl;
}
// orignal_map is map where the key are tuple who correcpond to hostname, process, ..., API call
// name, and the value are TallyCore
template <typename K, typename T, typename TC,
          typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
void print_compact(std::string title, std::unordered_map<K, TC> m, T &&keys_string,
                   int display_name_max_size) {

  if (m.empty())
    return;

  // Printing the summary of the number of Hostname, Process and co
  // We will iterator over the map and compute the number of uniq elements of each category
  auto tuple_tally = get_uniq_tally(m);
  print_tally(std::cout, title, tuple_tally, keys_string);
  std::cout << std::endl;

  auto aggregated_by_name = aggregate_by_name(m);
  print_tally2(aggregated_by_name, display_name_max_size);
}

// orignal_map is map where the key are tuple who correcpond to hostname, process, ..., API call
// name, and the value are TallyCore
template <typename K, typename T, typename TC,
          typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
void print_extended(std::string title, std::unordered_map<K, TC> m, T &&keys_string,
                    int display_name_max_size) {

  // Now working of the body of the table
  auto aggregated_nested = aggregate_nested(m);
  for (auto &[k, aggregated_by_name] : aggregated_nested) {
    print_named_tuple(std::cout, title, k, keys_string);
    std::cout << std::endl;
    print_tally2(aggregated_by_name, display_name_max_size);
  }
}

//        __  _
//     | (_  / \ |\ |
//   \_| __) \_/ | \|
//
// https://github.com/nlohmann/json
//
template <typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
void to_json(nlohmann::json &j, const TC &tc) {
  j = nlohmann::json{{"time", tc.duration}, {"call", tc.count}, {"min", tc.min}, {"max", tc.max}};
  if (tc.error != 0)
    j["error"] = tc.error;
}

template <typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
void to_json(nlohmann::json &j, const std::vector<std::pair<thapi_function_name, TC>> &aggregated) {
  for (auto const &[key, val] : aggregated)
    j[key] = val;
}

// orignal_map is map where the key are tuple who correcpond to hostname, process, ..., API call
// name, and the value are TallyCore
template <typename K, typename TC,
          typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
nlohmann::json json_compact(std::unordered_map<K, TC> &m) {
  auto aggregated_by_name = aggregate_by_name(m);
  auto sorted_by_value = sort_by_value(aggregated_by_name);
  add_footer(sorted_by_value);
  return {{"data", sorted_by_value}};
}

template <class... T, class... T2, size_t... I>
void json_populate(nlohmann::json &j, const std::tuple<T...> &h, const std::tuple<T2...> &s,
                   std::index_sequence<I...>) {
  ((j[std::get<I>(h)] = std::get<I>(s)), ...);
}

template <typename K, typename TC, class... T,
          typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
nlohmann::json json_extented(std::unordered_map<K, TC> &m, std::tuple<T...> &&h) {
  nlohmann::json j;
  auto aggregated_nested = aggregate_nested(m);
  for (auto &[k, aggregated_by_name] : aggregated_nested) {
    auto sorted_by_value = sort_by_value(aggregated_by_name);
    add_footer(sorted_by_value);

    nlohmann::json j2 = {{"data", sorted_by_value}};
    constexpr auto seq = std::make_index_sequence<sizeof...(T)>();
    json_populate(j2, h, k, seq);
    j.push_back(j2);
  }
  return j;
}

//
// Metadata
//
//
void print_metadata(std::vector<std::string> metadata) {
  if (metadata.empty())
    return;

  std::cout << "Metadata" << std::endl;
  std::cout << std::endl;
  for (std::string value : metadata)
    std::cout << value << std::endl;
}
