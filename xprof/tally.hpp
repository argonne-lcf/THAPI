#include <metababel/metababel.h>

#include <map>
#include <unordered_map>
#include <tuple>
#include <set>
#include <vector>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <regex>

#include "json.hpp"
#include "my_demangle.h"
#include "xprof_utils.hpp"


//! Returns a demangled name.
//! @param mangle_name function names
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
}

//! Returns a number as a string with the given number of decimals.
//! @param a_value the value to be casted to string with the given decimal places.
//! @param units units to be append at the end of the string.
//! @param n number of decimal places required.
//! REFERENCE: https://stackoverflow.com/questions/16605967/set-precision-of-stdto-string-when-converting-floating-point-values
template <typename T>
std::string to_string_with_precision(const T a_value, const std::string units, const int n = 2) {
  std::ostringstream out;
  out.precision(n);
  out << std::fixed << a_value << units;
  return out.str();
}

//! TallyCoreBase is a callbacks duration data collection and aggregation helper.
//! It is of interest to collect data for every (host,pid,tid,api_call_name) entity.
//! Since the same entity can take place several times, i.e., a thread spawned from 
//! a process running in a given host can call api_call_name several times, our 
//! interest is to aggregate these durations in a single one per entity.
//! In addition to the duration, other data of interest is collected from different
//! occurrences of the same entity such as, what was the minumum and max durations
//! among the occurrences, the number of times an api_call_name happened for a given
// "htp" (host,pid,tid), and how many occurrences failed.
//! Once the data of an entity is collected, when considering all its occurrences,
//! This helper calls facilitates aggregation of data by overloading += and + operators.
class TallyCoreBase {
public:
  TallyCoreBase() {}

  TallyCoreBase(uint64_t _dur, uint64_t _err) : duration{_dur}, error{_err} {
    count = 1;
    if (!error) {
      min = duration;
      max = duration;
    } else
      duration = 0;
  }

  uint64_t duration{0};
  uint64_t error{0};
  uint64_t min{std::numeric_limits<uint64_t>::max()};
  uint64_t max{0};
  uint64_t count{0};
  double duration_ratio{1.};
  double average{0};

  virtual const std::vector<std::string> to_string() = 0;

  const auto to_string_size() {
    std::vector<long> v;
    for (auto &e : to_string())
      v.push_back(static_cast<long>(e.size()));
    return v;
  }

  //! Accumulates duration information.
  TallyCoreBase &operator+=(const TallyCoreBase &rhs) {
    this->duration += rhs.duration;
    this->min = std::min(this->min, rhs.min);
    this->max = std::max(this->max, rhs.max);
    this->count += rhs.count;
    this->error += rhs.error;
    return *this;
  }

  //! Updates the average and duration ratio.
  //! NOTE: This should happened once we have collected the information the duration information
  //! of all the occurrences of a given (host,pid,tid,api_call_name) entity.
  void finalize(const TallyCoreBase &rhs) {
    average = ( count && count != error ) ? static_cast<double>(duration) / (count-error) : 0.;
    duration_ratio = static_cast<double>(duration) / rhs.duration;
  }

  //! Enables the comparison of two TallyCoreBase instances by their duration.
  //! It is used for sorting purposes.
  bool operator>(const TallyCoreBase &rhs) { return duration > rhs.duration; }

  void update_max_size(std::vector<long> &m) {
    const auto current_size = to_string_size();
    for (auto i = 0U; i < current_size.size(); i++)
      m[i] = std::max(m[i], current_size[i]);
  }
};

//! Specialization of TallyCoreBase for execution times.
class TallyCoreTime : public TallyCoreBase {
public:
  static constexpr std::array headers{"Time", "Time(%)", "Calls", "Average", "Min", "Max", "Error"};

  using TallyCoreBase::TallyCoreBase;
  virtual const std::vector<std::string> to_string() {
    return std::vector<std::string>{
      format_time(duration),
      std::isnan(duration_ratio) ? "" : to_string_with_precision(100. * duration_ratio, "%"),
      to_string_with_precision(count, "", 0),
      format_time(average),
      format_time(min),
      format_time(max),
      to_string_with_precision(error, "", 0)
    };
  }

private:
  //! Returns duration as a formatted string with units.
  template <typename T>
  std::string format_time(const T duration) {
    if (duration == std::numeric_limits<T>::max() || duration == T{0})
        return "";

    const double h = duration / 3.6e+12;
    if (h >= 1.)
      return to_string_with_precision(h, "h");

    const double min = duration / 6e+10;
    if (min >= 1.)
      return to_string_with_precision(min, "min");

    const double s = duration / 1e+9;
    if (s >= 1.)
      return to_string_with_precision(s, "s");

    const double ms = duration / 1e+6;
    if (ms >= 1.)
      return to_string_with_precision(ms, "ms");

    const double us = duration / 1e+3;
    if (us >= 1.)
      return to_string_with_precision(us, "us");

    return to_string_with_precision(duration, "ns");
  }
};

//! Specialization of TallyCoreBase for data transfer sizes.
//! This is used for traffic related events, lttng:traffic.
class TallyCoreByte : public TallyCoreBase {
public:
  static constexpr std::array headers{"Byte", "Byte(%)", "Calls", "Average", "Min", "Max", "Error"};

  using TallyCoreBase::TallyCoreBase;
  virtual const std::vector<std::string> to_string() {
    return std::vector<std::string>{
      format_byte(duration),
      to_string_with_precision(100. * duration_ratio, "%"),
      to_string_with_precision(count, "", 0),
      format_byte(average),
      format_byte(min),
      format_byte(max),
      to_string_with_precision(error, "", 0)
    };
  }

private:

  //! Returns a data transfer size (duration) as a formatted string with units.
  template <typename T> std::string format_byte(const T duration) {
    const double PB = duration / 1e+15;
    if (PB >= 1.)
      return to_string_with_precision(PB, "PB");

    const double TB = duration / 1e+12;
    if (TB >= 1.)
      return to_string_with_precision(TB, "TB");

    const double GB = duration / 1e+9;
    if (GB >= 1.)
      return to_string_with_precision(GB, "GB");

    const double MB = duration / 1e+6;
    if (MB >= 1.)
      return to_string_with_precision(MB, "MB");

    const double kB = duration / 1e+3;
    if (kB >= 1.)
      return to_string_with_precision(kB, "kB");

    return to_string_with_precision(duration, "B");
  }
};


//                  
//   | | _|_ o |  _ 
//   |_|  |_ | | _> 
//  

//! Join iterable items as string.
//! \param iterable an iterable container (set, map, etc) whose items support string concatenation.
//! \param delimiter :, ;, or other user specified delimiter.
//! \return Returns a string where iterable's "items" are separated by "delimiter"
template <typename T>
std::string join_iterator(const T& x, std::string delimiter = ",") {
    return std::accumulate( std::begin(x), std::end(x), std::string{},
                            [&delimiter](const std::string& a, const std::string &b ) {
                                  return a.empty() ? b: a + delimiter + b; } );
}


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
auto make_tuple_cuted(std::tuple<Args...> tp, std::index_sequence<Is...>) {
  return std::tuple{std::get<Is>(tp)...};
}

//! Remove the last element of a tuple.
/*!
\param tp (std::tuple).
\return Returns a new tuple without the last element.

EXAMPLE:
  input   ("iris01",232,789,"getDeviceInfo")
  output  ("iris01",232,789)

  It will create the following index sequence
  std::index_sequence<0,1,2>

  Because the "sizeof...(Args) - 1", the index_sequence discarded the last index.
  The created sequence is then passed to a helper that actually returns a new tuple
  containing the items in indexes 0,1,2.

REFERENCE: 
https://devblogs.microsoft.com/oldnewthing/20200623-00/?p=103901

*/
template <class... Args> 
auto make_tuple_cuted(std::tuple<Args...> tp) {
  return make_tuple_cuted(tp, std::make_index_sequence<sizeof...(Args) - 1>{});
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
template <typename TC, class... T, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
auto aggregate_nested(std::unordered_map<std::tuple<T...>, TC> &m) {

  // New type for a tuple without the last element.
  // Reference: https://stackoverflow.com/a/42043006/7674852
  typedef decltype(make_tuple_cuted(std::declval<std::tuple<T...>>())) Minusone;

  // Umap for the aggregated data
  std::unordered_map<Minusone, std::unordered_map<thapi_function_name, TC>> aggregated;

  for (auto &[key, val] : m) {
    auto head = make_tuple_cuted(key);
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
template <typename TC, class... T, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
auto aggregate_by_name(std::unordered_map<std::tuple<T...>, TC> &m) {
  std::unordered_map<thapi_function_name, TC> aggregated{};
  
  for (auto const &[key, val] : m)
    // use the thapi_function_name as the key.
    // thapi_function_name is the last element in the tuple "key".
    aggregated[std::get<sizeof...(T) - 1>(key)] += val;
  
  return aggregated;
}

//! Add the elements of the tuple "t" in the set elements "s".
/*! 
\param s tuple of sets.
\param t hpt_function_name_t (tuple)
\param index sequence

EXAMPLE:
  input  
    tuple{ set {}, set{}, set{}, set{} }, tuple{"iris01",232,789,"getDeviceInfo"}, std::index_sequence<0,1,2,3>
  output (update first param by reference) 
    tuple{ set {"iris01"}, set{232}, set{789}, set{"getDeviceInfo"} }

  The index sequence used as argument "std::index_sequence<0,1,2,3>" enables the following unfolding:

  (
    std::get<0>(s).insert(std::get<0>(t)),
    std::get<1>(s).insert(std::get<1>(t)),
    std::get<2>(s).insert(std::get<2>(t)),
    std::get<3>(s).insert(std::get<3>(t))
  );

  which adds every element of the tuple to its corresponding set, this updating the tuple of sets params
  provided by reference.

*/
template <class... T, class... T2, size_t... I>
void add_to_set(std::tuple<T...> &s, std::tuple<T2...> t, std::index_sequence<I...>) {
  (std::get<I>(s).insert(std::get<I>(t)), ...);
}

// NOTE: This function was placed for a reason, but we do not remember why.
// We saw it apparently do nothing, but we are not sure if we got a special
// case that needed this function treatment.
// We prefer to kept it until test show everything is working properly.
// template <class... T, size_t... I>
// void remove_neutral(std::tuple<std::set<T>...> &s, std::index_sequence<I...>) {
//   (std::get<I>(s).erase (T{}), ...);
// }

//! Add the elements of the tuple "t" in the set elements "s".
/*! This is used in the print_compact mode to know how many hosts, pids, tids, have been aggregated.
\param s tuple of sets.
\param t hpt_function_name_t (tuple)
\param index sequence
\return Returns a tuple of sets with unique hosts names, pids, tids, and api call function names found.

EXAMPLE:
  input   umap{ 
                ("iris01",232,789,"getDeviceInfo") : CoreTime,
                ("iris02",123,890,"getDeviceInfo") : CoreTime,
                ("iris02",890,890,"getDeviceInfo") : CoreTime 
          }
  output  tuple{ set {"iris01","iris02"}, set{232,123,890}, set{789,890}, set{"getDeviceInfo"} } 

TODO:
  Now, we are counting the number of unique elements in print_compact  using the .size method.
  Maybe we can do that here if no other function requires the result as is implemented right now.

*/
template <template <typename...> class Map, typename... K, typename V>
auto get_uniq_tally(Map<std::tuple<K...>, V> &input) {
  auto tuple_set = std::make_tuple(std::set<K>{}...);
  constexpr auto s = std::make_index_sequence<sizeof...(K)>();

  for (auto &m : input)
    add_to_set(tuple_set, m.first, s);

  // NOTE: This function was placed for a reason, but we do not remember why.
  // We saw it apparently do nothing, but we are not sure if we got a special
  // case that needed this function treatment.
  // We prefer to kept it until test show everything is working properly.
  // remove_neutral(tuple_set, s);

  return tuple_set;
}

//! Add the total values at the end of the table (represented as a vector of tuples)".
/*!
\param m vector of tuples.

EXAMPLE:
  input   vector{ 
                pair{"zeModuleCreate"),  CoreTime},
                pair{"zeModuleDestroy"), CoreTime},
                pair{"zeMemFree"),       CoreTime} 
          }
  output (update first param by reference) 

          vector{ 
                pair{"zeModuleCreate"),  CoreTime},
                pair{"zeModuleDestroy"), CoreTime},
                pair{"zeMemFree"),       CoreTime},
                pair{"Total"),           CoreTime}  
          }

*/
template <typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
void add_footer(std::vector<std::pair<thapi_function_name, TC>> &m) {

  // Create the final Tally
  TC tot{};
  for (auto const &keyval : m)
    tot += keyval.second;
  // Finalize (compute ratio)
  for (auto &keyval : m)
    keyval.second.finalize(tot);

  if (tot.error == tot.count)
    tot.duration_ratio = NAN;

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
template <std::size_t SIZE, typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
auto max_string_size(std::vector<std::pair<thapi_function_name, TC>> &m, const std::pair<std::string, std::array<const char *, SIZE>> header) {

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

// Our base constructor is a pair of either tuple / TalyCore and a vector of int correspond to the
// column size. We print each tuple members with the correct width, and joined with a `|`
// /!\ Ugly: If the column with is negative, that mean we should print a empty column of abs(size)
// This is useful for the footer or for hiding the `error` column

// We use 3 function, because my template skill are poor...
template <std::size_t SIZE>
std::ostream &operator<<(std::ostream &os,const std::pair<std::array<const char *, SIZE>, std::vector<long>> &_tup) {
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
void print_tally(std::ostream &os, const std::tuple<T...> &s, const std::tuple<T2...> &h, std::index_sequence<I...>) {
  ((std::get<I>(s).size() ? os << std::get<I>(s).size() << " " << std::get<I>(h) << " | "
                            : os << ""),
   ...);
}

template <class... T, class... T2>
void print_tally(std::ostream &os, const std::string &header, const std::tuple<T...> &s, const std::tuple<T2...> &h) {
  os << header << " | ";
  constexpr auto seq = std::make_index_sequence<sizeof...(T2)>();
  print_tally(os, s, h, seq);
  os << std::endl;
}

// Print 2 Tuple correspond to the hostname, process, ... device, subdevice.
template <class... T, class... T2, size_t... I>
void print_named_tuple(std::ostream &os, const std::tuple<T...> &s, const std::tuple<T2...> &h, std::index_sequence<I...>) {
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

// original_map is map where the key are tuple who correspond to hostname, process, ..., API call
// name, and the value are TallyCore
template <typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
void print_tally(std::unordered_map<thapi_function_name, TC> &m, int display_name_max_size) {

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
void print_compact(std::string title, std::unordered_map<K, TC> m, T &&keys_string, int display_name_max_size) {

  if (m.empty())
    return;

  // Printing the summary of the number of Hostname, Process and co
  // We will iterator over the map and compute the number of unique elements of each category
  auto tuple_tally = get_uniq_tally(m);
  print_tally(std::cout, title, tuple_tally, keys_string);
  std::cout << std::endl;

  auto aggregated_by_name = aggregate_by_name(m);
  print_tally(aggregated_by_name, display_name_max_size);
}

// original_map is map where the key are tuple who correspond to hostname, process, ..., API call
// name, and the value are TallyCore
template <typename K, typename T, typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
void print_extended(std::string title, std::unordered_map<K, TC> m, T &&keys_string, int display_name_max_size) {

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

// original_map is map where the key are tuple who correspond to hostname, process, ..., API call
// name, and the value are TallyCore
template <typename K, typename TC, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
nlohmann::json json_compact(std::unordered_map<K, TC> &m) {
  auto aggregated_by_name = aggregate_by_name(m);
  auto sorted_by_value = sort_by_value(aggregated_by_name);
  add_footer(sorted_by_value);
  return {{"data", sorted_by_value}};
}

template <class... T, class... T2, size_t... I>
void json_populate(nlohmann::json &j, const std::tuple<T...> &h, const std::tuple<T2...> &s, std::index_sequence<I...>) {
  ((j[std::get<I>(h)] = std::get<I>(s)), ...);
}

template <typename K, typename TC, class... T, typename = std::enable_if_t<std::is_base_of_v<TallyCoreBase, TC>>>
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
