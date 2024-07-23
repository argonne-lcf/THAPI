#pragma once

#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "json.hpp"
#include "tally_core.hpp"
#include "xprof_utils.hpp"

//! Returns a number as a string with the given number of decimals.
//! @param a_value the value to be casted to string with the given decimal places.
//! @param units units to be append at the end of the string.
//! @param n number of decimal places required.
//! REFERENCE:
//! https://stackoverflow.com/questions/16605967/set-precision-of-stdto-string-when-converting-floating-point-values

class TallyCoreString : public TallyCoreBase {

  using TallyCoreBase::TallyCoreBase;

protected:
  static constexpr size_t nfields = 7;
  template <typename T>
  std::string to_pretty_string(const T a_value, const std::string units, const int n = 2) {
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value << units;
    return out.str();
  }

public:
  void update_max_size(std::array<long, nfields> &m) {
    const auto current_string = to_string();
    for (auto i = 0UL; i < m.size(); i++)
        m[i] = std::max(m[i], static_cast<long>(current_string[i].size()));
    }
 
private:
  // Pure virtual function
  virtual const std::array<std::string, nfields> to_string() = 0;
};
//
//    /\   _   _  ._ _   _   _. _|_ o  _  ._
//   /--\ (_| (_| | (/_ (_| (_|  |_ | (_) | |
//         _|  _|        _|

//! Remove the last element of a tuple. (helper)
/*!
\param tp (std::tuple).
\return Returns a new tuple without the last element.
EXAMPLE:
    input   ("iris01",232,789,"getDeviceInfo"), std::index_sequence<0,1,2>
    output  ("iris01",232,789)

    It unfolds the expression "std::get<Is>(tp)..." according to Is.
    So it becomes, std::get<0>(tp), std::get<1>(tp), std::get<2>(tp).
    Then the final expression becomes

    "std::tuple{std::get<0>(tp), std::get<1>(tp), std::get<2>(tp)};"

REFERENCE:
https://devblogs.microsoft.com/oldnewthing/20200623-00/?p=103901

NOTE: Look like it may have some problem, but i was not smart enough
1/ to understand the problem
2/ To fix it

*/
template <class... Args, std::size_t... Is>
auto make_tuple_without_last(std::tuple<Args...> tp, std::index_sequence<Is...>) {
  return std::tuple{std::get<Is>(tp)...};
}

template <class... Args> auto make_tuple_without_last(std::tuple<Args...> tp) {
  return make_tuple_without_last(tp, std::make_index_sequence<sizeof...(Args) - 1>{});
}

//! Aggregate data by (host,pid,tid) and by (api_call_name)
/*! This is used eventually to print in extended mode, i.e., statistics per (host,pid,tid).
\param m (std::unordered_map).
\return Returns unordered map with data aggregated by (host,pid,tid)
    in the first level, and aggregated by api_call_name in the second level.

EXAMPLE:
    input   umap{ ("iris01",232,789,"getDeviceInfo") : CoreTime }
    output  umap{ ("iris01",232,789) : umap{ "getDeviceInfio" : CoreTime }
*/
template <typename TC, class... T,
          typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
auto aggregate_nested(std::unordered_map<std::tuple<T...>, TC> &m) {

  // New type for a tuple without the last element.
  // Reference: https://stackoverflow.com/a/42043006/7674852
  typedef decltype(make_tuple_without_last(std::declval<std::tuple<T...>>())) Minusone;

  // Umap for the aggregated data
  std::unordered_map<Minusone, std::unordered_map<thapi_function_name, TC>> aggregated;

  for (auto &[key, val] : m) {
    const auto head = make_tuple_without_last(key);
    aggregated[head][std::get<sizeof...(T) - 1>(key)] += val;
  }
  return aggregated;
}

//! Aggregate data by thapi_function_name.
/*! This is used eventually to print in compact mode, i.e., statistics per thapi_function_name.
\param m (std::unordered_map).
\return Returns unordered map with data aggregated by thapi_function_name.
EXAMPLE:
    input   umap{
                  ("iris01",232,789,"getDeviceInfo") : CoreTime,
                  ("iris02",123,890,"getDeviceInfo") : CoreTime
            }
    output  umap{ "getDeviceInfio" : CoreTime }
*/
template <typename TC, class... T,
          typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
auto aggregate_by_name(std::unordered_map<std::tuple<T...>, TC> &m) {
  std::unordered_map<thapi_function_name, TC> aggregated{};

  for (auto const &[key, val] : m)
    // use the thapi_function_name as the key.
    // thapi_function_name is the last element in the tuple "key".
    aggregated[std::get<sizeof...(T) - 1>(key)] += val;

  return aggregated;
}

/*
EXAMPLE:
  input   umap{
                ("iris01",232,789,"getDeviceInfo") : CoreTime,
                ("iris02",123,890,"getDeviceInfo") : CoreTime,
                ("iris02",890,890,"getDeviceInfo") : CoreTime
          }
  output  tuple{ set {"iris01","iris02"}, set{232,123,890}, set{789,890}, set{"getDeviceInfo"} }
*/
template <class... K,  size_t... I>
void add_to_set(std::tuple<std::set<K>...> &s,
                std::tuple<K...> t, std::index_sequence<I...>) {
  (std::get<I>(s).insert(std::get<I>(t)), ...);
}

template<typename... K, typename V>
auto get_uniq_tally(std::unordered_map<std::tuple<K...>, V> &input) {
  auto tuple_set = std::make_tuple(std::set<K>{}...);
  constexpr auto s = std::make_index_sequence<sizeof...(K)>();

  for (auto const &m : input)
    add_to_set(tuple_set, m.first, s);
  //return tuple_set;

  std::vector<size_t> a;
  auto N = (sizeof...(K));
  for (unsigned long i=0; i < N; i++) {
	  a.push_back(10);
  }
  return a;
}

template <typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
void add_footer(std::vector<std::pair<thapi_function_name, TC>> &m) {
  TC tot{};
  for (auto const &nt : m)
    tot += nt.second;
  m.push_back({"Total", tot});

  for (auto &nt : m)
    nt.second.compute_duration_ratio(tot);
}

//    __
//   (_   _  ._ _|_ o ._   _
//   __) (_) |   |_ | | | (_|
//                         _|
//

/* Take a map, and return a sorted vector of pair
 * sorted by original map values
 */

//! Take an umap and return a vector of pair sorted in descending order.
/*! This is used to sort by duration.
\param m unordered_map{thapi_function_name,TallyCoreBase}
\return Returns a vector of pairs with sorted entries.

EXAMPLE:
  input   umap{
                "zeModuleCreate"  , CoreTime{ duration = 20, ... },
                "zeModuleDestroy" , CoreTime{ duration = 15, ... },
                "getDeviceInfo"   , CoreTime{ duration = 30, ... }
          }
  output  vector{
                pair{ "zeModuleCreate"  , CoreTime{ duration = 30, ... } },
                pair{ "zeModuleDestroy" , CoreTime{ duration = 20, ... } },
                pair{ "getDeviceInfo"   , CoreTime{ duration = 15, ... } }
                ,

          }

*/
template <typename K, typename V> 
auto sort_by_value(const std::unordered_map<K, V> &m) {
  std::vector<std::pair<K, V>> v;
  std::copy(m.begin(), m.end(), std::back_inserter<std::vector<std::pair<K, V>>>(v));
  std::sort(v.begin(), v.end(), [=](auto &a, auto &b) { return a.second > b.second; });
  return v;
}

inline std::string limit_string_size(std::string original, int u_size, std::string j = "[...]") {
  if ((u_size < 0) || (static_cast<size_t>(u_size) >= original.length()))
    return original;

  if (static_cast<size_t>(u_size) <= j.length())
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
          typename = std::enable_if_t<std::is_base_of_v<TallyCoreString, TC>>>
auto max_string_size(std::vector<std::pair<thapi_function_name, TC>> &m,
                     const std::pair<std::string, std::array<const char *, SIZE>> header) {

  auto &[header_name, header_tallycore] = header;
  long name_max = header_name.size();

  // Know at compile time
  auto tallycore_max =
      std::apply([](auto &&...e) { return std::array{(static_cast<long>(strlen(e)))...}; },
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

// Our base constructor is a pair of either tuple / TalyCore and a vector of int correspond to the
// column size. We print each tuple members with the correct width, and joined with a `|`
// /!\ Ugly: If the column with is negative, that mean we should print a empty column of abs(size)
// This is useful for the footer or for hiding the `error` column

// We use 3 function, because my template skill are poor...
template <size_t SIZE>
std::ostream &operator<<(std::ostream &os,
                         const std::pair<std::array<const char *, SIZE>, std::array<long, SIZE>> &_tup) {
  auto &[c, column_width] = _tup;
  for (auto i = 0U; i < c.size(); i++) {
    os << std::setw(std::abs(column_width[i]));
    if (column_width[i] <= 0)
      os << ""
         << "   ";
    else
      os << c[i] << " | ";
  }
  return os;
}

// Print the TallyCore
template <size_t SIZE, typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreString, TC>>>
std::ostream &operator<<(std::ostream &os, std::pair<TC, std::array<long, SIZE>> &_tup) {
  auto &[c, column_width] = _tup;
  const auto v = c.to_string();
  for (auto i = 0U; i < v.size(); i++) {
    os << std::setw(std::abs(column_width[i]));
    if (column_width[i] <= 0)
      os << ""
         << "   ";
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
template <class T, class... T2, size_t... I>
void print_tally_header(std::ostream &os, const std::vector<T> &s, const std::tuple<T2...> &h,
                 std::index_sequence<I...>) {
  (( (I < s.size() ) ? os << s[I] << " " << std::get<I>(h) << " | "
                          : os << ""),
   ...);
}

template <class T, class... T2>
void print_tally_header(std::ostream &os, const std::string &header, const std::vector<T> &s,
                 const std::tuple<T2...> &h) {
  os << header << " | ";
  constexpr auto seq = std::make_index_sequence<sizeof...(T2)>();
  print_tally_header(os, s, h, seq);
  os << std::endl;
}

// Print 2 Tuple correspond to the hostname, process, ... device, subdevice.
template <class... T, class... T2, size_t... I>
void print_named_tuple(std::ostream &os, const std::tuple<T...> &s, const std::tuple<T2...> &h,
                       std::index_sequence<I...>) {
  (((std::get<I>(s) != T{}) ? os << std::get<I>(h) << ": " << std::get<I>(s) << " | " : os << ""),
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

// original_map is map where the key are tuple who correspond to hostname, process, ..., API call
// name, and the value are TallyCore
template <typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
void print_tally(const std::unordered_map<thapi_function_name, TC> &m, int display_name_max_size) {

  auto sorted_by_value = sort_by_value(m);
  add_footer(sorted_by_value);
  apply_sizelimit(sorted_by_value, display_name_max_size);

  const auto header = std::make_pair(std::string("Name"), TC::headers);

  auto &&[s1, s2] = max_string_size(sorted_by_value, header);

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
// original_map is map where the key are tuple who correspond to hostname, process, ..., API call
// name, and the value are TallyCore
template <typename K, typename T, typename TC,
          typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
void print_compact(std::string title, std::unordered_map<K, TC> m, T &&keys_string,
                   int display_name_max_size) {

  if (m.empty())
    return;

  // Printing the summary of the number of Hostname, Process and co
  // We will iterator over the map and compute the number of unique elements of each category
  const auto tuple_tally = get_uniq_tally(m);
  print_tally_header(std::cout, title, tuple_tally, keys_string);
  std::cout << std::endl;

  auto aggregated_by_name = aggregate_by_name(m);
  print_tally(aggregated_by_name, display_name_max_size);
}

// original_map is map where the key are tuple who correspond to hostname, process, ..., API call
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
    print_tally(aggregated_by_name, display_name_max_size);
  }
}

//        __  _
//     | (_  / \ |\ |
//   \_| __) \_/ | \|
//
// https://github.com/nlohmann/json
//
template <typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreString, TC>>>
void to_json(nlohmann::json &j, const TC &tc) {
  j = nlohmann::json{{"time", tc.duration}, {"call", tc.count}, {"min", tc.min}, {"max", tc.max}};
  if (tc.error != 0)
    j["error"] = tc.error;
}

template <typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreString, TC>>>
void to_json(nlohmann::json &j, const std::vector<std::pair<thapi_function_name, TC>> &aggregated) {
  for (auto const &[key, val] : aggregated)
    j[key] = val;
}

// original_map is map where the key are tuple who correspond to hostname, process, ..., API call
// name, and the value are TallyCore
template <typename K, typename TC,
          typename = std::enable_if_t<std::is_base_of_v<TallyCoreString, TC>>>
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
          typename = std::enable_if_t<std::is_base_of_v<TallyCoreString, TC>>>
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


// Wrapper type that does reversal
template <typename Range>
class Reverser {
    Range& r_;
  public:
    using iterator_type = std::reverse_iterator<decltype(std::begin(r_))>;

    Reverser(Range& r) : r_(r) {}

    iterator_type begin() { return iterator_type(std::end(r_)); }
    iterator_type end()   { return iterator_type(std::begin(r_)); }
};

// Helper creation function
template <typename Range>
Reverser<Range> reverse(Range& r)
{
    return Reverser<Range>(r);
}
