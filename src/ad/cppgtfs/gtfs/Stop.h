// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#ifndef AD_CPPGTFS_GTFS_STOP_H_
#define AD_CPPGTFS_GTFS_STOP_H_

#include <stdint.h>
#include <cassert>
#include <string>

using std::exception;
using std::string;

namespace ad {
namespace cppgtfs {
namespace gtfs {

class Stop {
 public:
  enum LOCATION_TYPE : uint8_t { STOP = 0, STATION = 1, STATION_ENTRANCE = 2 };

  enum WHEELCHAIR_BOARDING : uint8_t {
    NO_INFORMATION = 0,
    BOARDING_POSSIBLE = 1,
    BOARDING_NOT_POSSIBLE = 2
  };

  Stop() {}

  Stop(const string& id, const string& code, const string& name,
       const string& desc, double lat, double lng, string zone_id,
       const string& stop_url, Stop::LOCATION_TYPE location_type,
       Stop* parent_station, const string& stop_timezone,
       Stop::WHEELCHAIR_BOARDING wheelchair_boarding, const std::string& platform_code)
      : _id(id),
        _code(code),
        _name(name),
        _desc(desc),
        _zone_id(zone_id),
        _stop_url(stop_url),
        _stop_timezone(stop_timezone),
        _platform_code(platform_code),
        _parent_station(parent_station),
        _lat(lat),
        _lng(lng),
        _wheelchair_boarding(wheelchair_boarding),
        _location_type(location_type) {}

  const std::string& getId() const { return _id; }

  const std::string& getCode() const { return _code; }

  const std::string& getName() const { return _name; }

  const std::string& getPlatformCode() const { return _platform_code; }

  const std::string& getDesc() const { return _desc; }

  double getLat() const { return _lat; }

  double getLng() const { return _lng; }

  const std::string& getZoneId() const { return _zone_id; }

  const std::string& getStopUrl() const { return _stop_url; }

  Stop::LOCATION_TYPE getLocationType() const { return _location_type; }

  const Stop* getParentStation() const { return _parent_station; }

  Stop* getParentStation() { return _parent_station; }

  void setParentStation(Stop* p) { _parent_station = p; }

  const std::string& getStopTimezone() const { return _stop_timezone; }

  Stop::WHEELCHAIR_BOARDING getWheelchairBoarding() const {
    return _wheelchair_boarding;
  }

  // TODO(patrick): implement setters

 private:
  string _id, _code, _name, _desc, _zone_id, _stop_url, _stop_timezone,
      _platform_code;
  Stop* _parent_station;
  double _lat, _lng;
  Stop::WHEELCHAIR_BOARDING _wheelchair_boarding;
  Stop::LOCATION_TYPE _location_type;
};

}  // namespace gtfs
}  // namespace cppgtfs
}  // namespace ad

#endif  // AD_CPPGTFS_GTFS_STOP_H_
