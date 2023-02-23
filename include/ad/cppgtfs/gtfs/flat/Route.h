// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef AD_CPPGTFS_GTFS_FLAT_ROUTE_H_
#define AD_CPPGTFS_GTFS_FLAT_ROUTE_H_

#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <set>
#include <algorithm>

namespace ad {
namespace cppgtfs {
namespace gtfs {
namespace flat {

struct RouteFlds {
  size_t routeIdFld;
  size_t routeLongNameFld;
  size_t routeShortNameFld;
  size_t routeTypeFld;
  size_t routeUrlFld;
  size_t routeDescFld;
  size_t agencyIdFld;
  size_t routeColorFld;
  size_t routeTextColorFld;
  size_t routeSortOrderFld;
  size_t continuousDropOffFld;
  size_t continuousPickupFld;
};

struct Route {
  enum TYPE : uint8_t {
    TRAM = 0,
    SUBWAY = 1,
    RAIL = 2,
    BUS = 3,
    FERRY = 4,
    CABLE_CAR = 5,
    GONDOLA = 6,
    FUNICULAR = 7,
    TROLLEYBUS = 11,
    MONORAIL = 12,
    COACH = 200,
    AIR = 199,
    NONE = 99
  };

  std::string id;
  std::string agency;
  std::string short_name;
  std::string long_name;
  std::string desc;
  Route::TYPE type;
  std::string url;
  uint32_t color;
  uint32_t text_color;
  int64_t sort_order;
  uint8_t continuous_pickup;
  uint8_t continuous_drop_off;

  static std::string getHexColorString(uint32_t color) {
    if (color == std::numeric_limits<uint32_t>::max()) return "";
    // using stringstream here, because it doesnt add "0x" to the front
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(6) << color;
    return ss.str();
  }

  static std::string getTypeString(flat::Route::TYPE t) {
    if (t == flat::Route::COACH) return "coach";
    if (t == flat::Route::TROLLEYBUS) return "trolleybus";
    if (t == flat::Route::MONORAIL) return "monorail";
    std::string names[8] = {"tram",  "subway",   "rail",    "bus",
                            "ferry", "cablecar", "gondola", "funicular"};
    return names[t];
  }

  static flat::Route::TYPE getRouteType(int t) {
    switch (t) {
      case 2:
      case 100:
      case 101:
      case 102:
      case 103:
      case 104:
      case 105:
      case 106:
      case 107:
      case 108:
      case 109:
      case 110:
      case 111:
      case 112:
      case 113:
      case 114:
      case 115:
      case 117:
      case 300:
      case 400:
      case 403:
      case 404:
      case 1503:
        return Route::TYPE::RAIL;
      case 3:
        return Route::TYPE::BUS;
      case 200:
      case 201:
      case 202:
      case 203:
      case 204:
      case 205:
      case 206:
      case 207:
      case 208:
      case 209:
        return Route::TYPE::COACH;
      case 700:
      case 701:
      case 702:
      case 703:
      case 704:
      case 705:
      case 706:
      case 707:
      case 708:
      case 709:
      case 710:
      case 711:
      case 712:
      case 713:
      case 714:
      case 715:
      case 716:
      case 717:
      case 1500:
      case 1501:
      case 1505:
      case 1506:
      case 1507:
        return Route::TYPE::BUS;
      case 1:
      case 401:
      case 402:
      case 500:
      case 600:
        return Route::TYPE::SUBWAY;
      case 0:
      case 900:
      case 901:
      case 902:
      case 903:
      case 904:
      case 905:
      case 906:
        return Route::TYPE::TRAM;
      case 4:
      case 1000:
      case 1001:
      case 1002:
      case 1003:
      case 1004:
      case 1005:
      case 1006:
      case 1007:
      case 1008:
      case 1009:
      case 1010:
      case 1011:
      case 1012:
      case 1013:
      case 1014:
      case 1015:
      case 1016:
      case 1017:
      case 1018:
      case 1019:
      case 1020:
      case 1021:
      case 1200:
      case 1502:
        return Route::TYPE::FERRY;
      // TODO(patrick): from here on not complete!
      case 6:
      case 1300:
      case 1301:
      case 1304:
      case 1306:
      case 1307:
        return Route::TYPE::GONDOLA;
      case 1101:
      case 1102:
      case 1103:
      case 1104:
      case 1105:
      case 1106:
      case 1107:
      case 1108:
      case 1109:
      case 1110:
      case 1111: case 1112:
      case 1113:
      case 1114:
        return Route::TYPE::GONDOLA;
      case 7:
      case 116:
      case 1303:
      case 1302:
      case 1400:
        return Route::TYPE::FUNICULAR;
      case 5:
        return Route::TYPE::CABLE_CAR;
      case 11:
      case 800:
        return Route::TYPE::TROLLEYBUS;
      case 12:
      case 405:
        return Route::TYPE::MONORAIL;
      default:
        return Route::TYPE::NONE;
    }
  }

  static std::set<flat::Route::TYPE> getTypesFromString(std::string name) {
    std::set<flat::Route::TYPE> ret;

    if (name.empty()) return ret;

    char* rem;
    uint64_t num = strtol(name.c_str(), &rem, 10);
    if (!*rem) {
      auto i = getRouteType(num);
      if (i != Route::TYPE::NONE) ret.insert(i);
      return ret;
    }

    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (name == "all") {
      ret.insert(flat::Route::TYPE::TRAM);
      ret.insert(flat::Route::TYPE::SUBWAY);
      ret.insert(flat::Route::TYPE::RAIL);
      ret.insert(flat::Route::TYPE::BUS);
      ret.insert(flat::Route::TYPE::COACH);
      ret.insert(flat::Route::TYPE::FERRY);
      ret.insert(flat::Route::TYPE::CABLE_CAR);
      ret.insert(flat::Route::TYPE::GONDOLA);
      ret.insert(flat::Route::TYPE::FUNICULAR);
      ret.insert(flat::Route::TYPE::TROLLEYBUS);
      ret.insert(flat::Route::TYPE::MONORAIL);
      return ret;
    }

    if (name == "bus") {
      ret.insert(flat::Route::TYPE::BUS);
      return ret;
    }

    if (name == "tram" || name == "streetcar" || name == "light_rail" ||
        name == "lightrail" || name == "light-rail") {
      ret.insert(flat::Route::TYPE::TRAM);
      return ret;
    }

    if (name == "train" || name == "rail") {
      ret.insert(flat::Route::TYPE::RAIL);
      return ret;
    }

    if (name == "ferry" || name == "boat" || name == "ship") {
      ret.insert(flat::Route::TYPE::FERRY);
      return ret;
    }

    if (name == "subway" || name == "metro") {
      ret.insert(flat::Route::TYPE::SUBWAY);
      return ret;
    }

    if (name == "cablecar" || name == "cable_car" || name == "cable-car") {
      ret.insert(flat::Route::TYPE::CABLE_CAR);
      return ret;
    }

    if (name == "gondola") {
      ret.insert(flat::Route::TYPE::GONDOLA);
      return ret;
    }

    if (name == "air") {
      ret.insert(flat::Route::TYPE::AIR);
      return ret;
    }

    if (name == "funicular") {
      ret.insert(flat::Route::TYPE::FUNICULAR);
      return ret;
    }

    if (name == "coach") {
      ret.insert(flat::Route::TYPE::COACH);
      return ret;
    }

    if (name == "mono-rail" || name == "monorail") {
      ret.insert(flat::Route::TYPE::MONORAIL);
      return ret;
    }

    if (name == "trolley" || name == "trolleybus" || name == "trolley-bus") {
      ret.insert(flat::Route::TYPE::TROLLEYBUS);
      return ret;
    }

    return ret;
  }
};

}  // namespace flat
}  // namespace gtfs
}  // namespace cppgtfs
}  // namespace ad

#endif  // AD_CPPGTFS_GTFS_ROUTE_H_
