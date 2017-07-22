// Copyright 2016, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Authors: Patrick Brosi <brosi@informatik.uni-freiburg.de>

#include <boost/filesystem.hpp>
#include <fstream>
#include <map>
#include <string>
#include <utility>
#include "Parser.h"
#include "ad/util/CsvParser.h"
#include "gtfs/Agency.h"
#include "gtfs/Route.h"
#include "gtfs/Stop.h"

using ad::util::CsvParser;
using ad::util::CsvParserException;
using ad::cppgtfs::Parser;
using ad::cppgtfs::gtfs::Agency;
using ad::cppgtfs::gtfs::Stop;
using ad::cppgtfs::gtfs::Route;
using ad::cppgtfs::gtfs::Trip;
using ad::cppgtfs::gtfs::Service;
using ad::cppgtfs::gtfs::ServiceDate;
using ad::cppgtfs::gtfs::StopTime;
using ad::cppgtfs::gtfs::Shape;
using ad::cppgtfs::gtfs::ShapePoint;
using ad::cppgtfs::gtfs::Time;

// ____________________________________________________________________________
bool Parser::parse(gtfs::Feed* targetFeed, std::string path) const {
  std::ifstream fs;
  boost::filesystem::path gtfsPath(path);
  boost::filesystem::path curFile;

  try {
    curFile = gtfsPath / "agency.txt";
    fs.open(curFile.c_str());
    if (!fs.good()) fileNotFound(curFile);
    parseAgency(targetFeed, &fs);
    fs.close();

    curFile = gtfsPath / "stops.txt";
    fs.open(curFile.c_str());
    if (!fs.good()) fileNotFound(curFile);
    parseStops(targetFeed, &fs);
    fs.close();

    curFile = gtfsPath / "routes.txt";
    fs.open(curFile.c_str());
    if (!fs.good()) fileNotFound(curFile);
    parseRoutes(targetFeed, &fs);
    fs.close();

    curFile = gtfsPath / "calendar.txt";
    fs.open(curFile.c_str());
    if (fs.good()) {
      parseCalendar(targetFeed, &fs);
      fs.close();
    }

    curFile = gtfsPath / "calendar_dates.txt";
    fs.open(curFile.c_str());
    if (fs.good()) {
      parseCalendarDates(targetFeed, &fs);
      fs.close();
    }

    curFile = gtfsPath / "shapes.txt";
    fs.open(curFile.c_str());
    if (fs.good()) {
      parseShapes(targetFeed, &fs);
      fs.close();
    }

    curFile = gtfsPath / "trips.txt";
    fs.open(curFile.c_str());
    if (!fs.good()) fileNotFound(curFile);
    parseTrips(targetFeed, &fs);
    fs.close();

    curFile = gtfsPath / "stop_times.txt";
    fs.open(curFile.c_str());
    if (!fs.good()) fileNotFound(curFile);
    parseStopTimes(targetFeed, &fs);
    fs.close();
  } catch (const CsvParserException& e) {
    throw ParserException(e.getMsg(), e.getFieldName(), e.getLine(),
                          curFile.c_str());
  } catch (const ParserException& e) {
    // augment with file namoe
    ParserException fe = e;
    fe.setFileName(curFile.c_str());
    throw fe;
  }

  return true;
}

// ____________________________________________________________________________
void Parser::parseAgency(gtfs::Feed* targetFeed, std::istream* s) const {
  CsvParser csvp(s);
  Agency* a = 0;

  while (csvp.readNextLine()) {
    a = new gtfs::Agency(
        getString(csvp, "agency_id", ""), getString(csvp, "agency_name"),
        getString(csvp, "agency_url"), getString(csvp, "agency_timezone"),
        getString(csvp, "agency_lang", ""), getString(csvp, "agency_phone", ""),
        getString(csvp, "agency_fare_url", ""),
        getString(csvp, "agency_email", ""));

    if (!targetFeed->getAgencies().add(a)) {
      std::stringstream msg;
      msg << "'agency_id' must be dataset unique. Collision with id '"
          << a->getId() << "')";
      throw ParserException(msg.str(), "agency_id", csvp.getCurLine());
    }
  }

  if (!a) {
    throw ParserException(
        "the feed has no agency defined."
        " This is a required field.",
        "", 1);
  }
}

// ____________________________________________________________________________
void Parser::parseStops(gtfs::Feed* targetFeed, std::istream* s) const {
  CsvParser csvp(s);

  std::map<Stop*, std::pair<size_t, std::string> > parentStations;

  while (csvp.readNextLine()) {
    Stop::LOCATION_TYPE locType = static_cast<Stop::LOCATION_TYPE>(
        getRangeInteger(csvp, "location_type", 0, 2, 0));

    Stop* s = new Stop(
        getString(csvp, "stop_id"), getString(csvp, "stop_code", ""),
        getString(csvp, "stop_name"), getString(csvp, "stop_desc", ""),
        getDouble(csvp, "stop_lat"), getDouble(csvp, "stop_lon"),
        getString(csvp, "zone_id", ""), getString(csvp, "stop_url", ""),
        locType, 0, getString(csvp, "stop_timezone", ""),
        static_cast<Stop::WHEELCHAIR_BOARDING>(
            getRangeInteger(csvp, "wheelchair_boarding", 0, 2, 0)));

    const std::string& parentStatId = getString(csvp, "parent_station", "");
    if (!parentStatId.empty()) {
      if (locType == Stop::LOCATION_TYPE::STATION) {
        throw ParserException(
            "a stop with location_type 'station' (1) cannot"
            " have a parent station",
            "parent_station", csvp.getCurLine());
      }

      parentStations[s] =
          std::pair<size_t, std::string>(csvp.getCurLine(), parentStatId);
    }

    if (!targetFeed->getStops().add(s)) {
      std::stringstream msg;
      msg << "'stop_id' must be dataset unique. Collision with id '"
          << s->getId() << "')";
      throw ParserException(msg.str(), "stop_id", csvp.getCurLine());
    }
  }

  // second pass to resolve parentStation pointers
  for (const auto& ps : parentStations) {
    Stop* parentStation = 0;
    parentStation = targetFeed->getStops().get(ps.second.second);
    if (!parentStation) {
      std::stringstream msg;
      msg << "no stop with id '" << ps.second.second << "' defined, cannot "
          << "reference here.";
      throw ParserException(msg.str(), "parent_station", ps.second.first);
    } else {
      ps.first->setParentStation(parentStation);
    }
  }
}

// ____________________________________________________________________________
void Parser::parseRoutes(gtfs::Feed* targetFeed, std::istream* s) const {
  CsvParser csvp(s);

  while (csvp.readNextLine()) {
    std::string agencyId = getString(csvp, "agency_id", "");
    Agency* routeAgency = 0;

    if (!agencyId.empty()) {
      routeAgency = targetFeed->getAgencies().get(agencyId);
      if (!routeAgency) {
        std::stringstream msg;
        msg << "no agency with id '" << agencyId << "' defined, cannot "
            << "reference here.";
        throw ParserException(msg.str(), "agency_id", csvp.getCurLine());
      }
    }

    Route* r = new Route(
        getString(csvp, "route_id"), routeAgency,
        getString(csvp, "route_short_name", ""),
        getString(csvp, "route_long_name"), getString(csvp, "route_desc", ""),
        getRouteType(csvp, "route_type",
                     getRangeInteger(csvp, "route_type", 0, 1702)),
        getString(csvp, "route_url", ""),
        getColorFromHexString(csvp, "route_color", "FFFFFF"),
        getColorFromHexString(csvp, "route_text_color", "000000"));

    if (!targetFeed->getRoutes().add(r)) {
      std::stringstream msg;
      msg << "'route_id' must be dataset unique. Collision with id '"
          << r->getId() << "')";
      throw ParserException(msg.str(), "route_id", csvp.getCurLine());
    }
  }
}

// ____________________________________________________________________________
void Parser::parseCalendar(gtfs::Feed* targetFeed, std::istream* s) const {
  CsvParser csvp(s);

  while (csvp.readNextLine()) {
    std::string serviceId = getString(csvp, "service_id");

    Service* s = new Service(
        serviceId, (0 << getRangeInteger(csvp, "monday", 0, 1)) |
                       (0 << getRangeInteger(csvp, "tuesday", 0, 1) * 2) |
                       (0 << getRangeInteger(csvp, "wednesday", 0, 1) * 3) |
                       (0 << getRangeInteger(csvp, "thursday", 0, 1) * 4) |
                       (0 << getRangeInteger(csvp, "friday", 0, 1) * 5) |
                       (0 << getRangeInteger(csvp, "saturday", 0, 1) * 6) |
                       (0 << getRangeInteger(csvp, "sunday", 0, 1) * 7),
        getServiceDate(csvp, "start_date"), getServiceDate(csvp, "end_date"));

    if (!targetFeed->getServices().add(s)) {
      std::stringstream msg;
      msg << "'service_id' must be unique in calendars.txt. Collision with id '"
          << s->getId() << "')";
      throw ParserException(msg.str(), "service_id", csvp.getCurLine());
    }
  }
}

// ____________________________________________________________________________
void Parser::parseCalendarDates(gtfs::Feed* targetFeed, std::istream* s) const {
  CsvParser csvp(s);

  while (csvp.readNextLine()) {
    std::string serviceId = getString(csvp, "service_id");
    ServiceDate d = getServiceDate(csvp, "date");
    Service::EXCEPTION_TYPE t = static_cast<Service::EXCEPTION_TYPE>(
        getRangeInteger(csvp, "exception_type", 1, 2));

    Service* e = targetFeed->getServices().get(serviceId);

    if (!e) {
      e = new Service(serviceId);
      targetFeed->getServices().add(e);
    }

    e->addException(d, t);
  }
}

// ____________________________________________________________________________
void Parser::parseTrips(gtfs::Feed* targetFeed, std::istream* s) const {
  CsvParser csvp(s);

  while (csvp.readNextLine()) {
    std::string routeId = getString(csvp, "route_id");
    Route* tripRoute = 0;

    tripRoute = targetFeed->getRoutes().get(routeId);
    if (!tripRoute) {
      std::stringstream msg;
      msg << "no route with id '" << routeId << "' defined, cannot "
          << "reference here.";
      throw ParserException(msg.str(), "route_id", csvp.getCurLine());
    }

    std::string shapeId = getString(csvp, "shape_id", "");
    Shape* tripShape = 0;

    if (!shapeId.empty()) {
      tripShape = targetFeed->getShapes().get(shapeId);
      if (!tripShape) {
        std::stringstream msg;
        msg << "no shape with id '" << shapeId << "' defined, cannot "
            << "reference here.";
        throw ParserException(msg.str(), "shape_id", csvp.getCurLine());
      }
    }

    std::string serviceId = getString(csvp, "service_id");
    Service* tripService = targetFeed->getServices().get(serviceId);
    if (!tripService) {
      std::stringstream msg;
      msg << "no service with id '" << serviceId << "' defined, cannot "
          << "reference here.";
      throw ParserException(msg.str(), "service_id", csvp.getCurLine());
    }

    Trip* t = new Trip(getString(csvp, "trip_id"), tripRoute, tripService,
                       getString(csvp, "trip_headsign", ""),
                       getString(csvp, "trip_short_name", ""),
                       static_cast<Trip::DIRECTION>(
                           getRangeInteger(csvp, "direction_id", 0, 1, 2)),
                       getString(csvp, "block_id", ""), tripShape,
                       static_cast<Trip::WC_BIKE_ACCESSIBLE>(getRangeInteger(
                           csvp, "wheelchair_accessible", 0, 2, 0)),
                       static_cast<Trip::WC_BIKE_ACCESSIBLE>(
                           getRangeInteger(csvp, "bikes_allowed", 0, 2, 0)));

    if (!targetFeed->getTrips().add(t)) {
      std::stringstream msg;
      msg << "'trip_id' must be dataset unique. Collision with id '"
          << t->getId() << "')";
      throw ParserException(msg.str(), "trip_id", csvp.getCurLine());
    }
  }
}

// ____________________________________________________________________________
void Parser::parseShapes(gtfs::Feed* targetFeed, std::istream* s) const {
  CsvParser csvp(s);

  while (csvp.readNextLine()) {
    const std::string& shapeId = getString(csvp, "shape_id");
    Shape* s = targetFeed->getShapes().get(shapeId);
    if (!s) {
      targetFeed->getShapes().add(new Shape(shapeId));
      s = targetFeed->getShapes().get(shapeId);
    }

    std::string rawDist = getString(csvp, "shape_dist_traveled", "");

    double dist = -1;  // using -1 as a NULL value here

    if (!rawDist.empty()) {
      dist = getDouble(csvp, "shape_dist_traveled");
      if (dist < -0.01) {  // TODO(patrick): better double comp
        throw ParserException(
            "negative values not supported for distances"
            " (value was: " +
                std::to_string(dist),
            "shape_dist_traveled", csvp.getCurLine());
      }
    }

    if (!s->addPoint(ShapePoint(
            getDouble(csvp, "shape_pt_lat"), getDouble(csvp, "shape_pt_lon"),
            dist, getRangeInteger(csvp, "shape_pt_sequence", 0, UINT32_MAX)))) {
      throw ParserException(
          "shape_pt_sequence collision,"
          "shape_pt_sequence has "
          "to be increasing for a single shape.",
          "shape_pt_sequence", csvp.getCurLine());
    }
  }
}

// ____________________________________________________________________________
void Parser::parseStopTimes(gtfs::Feed* targetFeed, std::istream* s) const {
  CsvParser csvp(s);

  while (csvp.readNextLine()) {
    Stop* stop = 0;
    Trip* trip = 0;

    const std::string& stopId = getString(csvp, "stop_id");
    const std::string& tripId = getString(csvp, "trip_id");

    stop = targetFeed->getStops().get(stopId);
    trip = targetFeed->getTrips().get(tripId);

    if (!stop) {
      std::stringstream msg;
      msg << "no stop with id '" << stopId << "' defined in stops.txt, cannot "
          << "reference here.";
      throw ParserException(msg.str(), "stop_id", csvp.getCurLine());
    }

    if (!trip) {
      std::stringstream msg;
      msg << "no trip with id '" << tripId << "' defined in trips.txt, cannot "
          << "reference here.";
      throw ParserException(msg.str(), "trip_id", csvp.getCurLine());
    }

    std::string rawDist = getString(csvp, "shape_dist_traveled", "");

    double dist = -1;  // using -1 as a NULL value here

    if (!rawDist.empty()) {
      dist = getDouble(csvp, "shape_dist_traveled");
      if (dist < -0.01) {  // TODO(patrick): better double comp
        throw ParserException(
            "negative values not supported for distances"
            " (value was: " +
                std::to_string(dist),
            "shape_dist_traveled", csvp.getCurLine());
      }
    }

    StopTime st(getTime(csvp, "arrival_time"), getTime(csvp, "departure_time"),
                stop, getRangeInteger(csvp, "stop_sequence", 0, UINT32_MAX),
                getString(csvp, "stop_headsign", ""),
                static_cast<StopTime::PU_DO_TYPE>(
                    getRangeInteger(csvp, "drop_off_type", 0, 3, 0)),
                static_cast<StopTime::PU_DO_TYPE>(
                    getRangeInteger(csvp, "pick_up_type", 0, 3, 0)),
                dist, getRangeInteger(csvp, "timepoint", 0, 1, 1));

    if (st.getArrivalTime() > st.getDepartureTime()) {
      throw ParserException("arrival time '" + st.getArrivalTime().toString() +
                                "' is later than departure time '" +
                                st.getDepartureTime().toString() +
                                "'. You cannot depart earlier than you arrive.",
                            "departure_time", csvp.getCurLine());
    }

    if (!trip->addStopTime(st)) {
      throw ParserException(
          "stop_sequence collision, stop_sequence has "
          "to be increasing for a single trip.",
          "stop_sequence", csvp.getCurLine());
    }
  }
}

// ___________________________________________________________________________
void Parser::fileNotFound(boost::filesystem::path file) const {
  throw ParserException("File not found", "", -1, std::string(file.c_str()));
}

// ___________________________________________________________________________
std::string Parser::getString(const CsvParser& csv,
                              const std::string& field) const {
  return csv.getTString(field.c_str());
}

// ___________________________________________________________________________
std::string Parser::getString(const CsvParser& csv, const std::string& fld,
                              const std::string& def) const {
  std::string ret = def;

  if (csv.hasItem(fld.c_str()) && !csv.fieldIsEmpty(fld.c_str())) {
    ret = csv.getTString(fld.c_str());
  }

  return ret;
}

// ___________________________________________________________________________
double Parser::getDouble(const CsvParser& csv, const std::string& field) const {
  return csv.getDouble(field.c_str());
}

// ___________________________________________________________________________
double Parser::getDouble(const CsvParser& csv, const std::string& field,
                         double ret) const {
  if (csv.hasItem(field.c_str()) && !csv.fieldIsEmpty(field.c_str())) {
    ret = csv.getDouble(field.c_str());
  }

  return ret;
}

// ___________________________________________________________________________
int64_t Parser::getRangeInteger(const CsvParser& csv, const std::string& field,
                                int64_t minv, int64_t maxv) const {
  int64_t ret = csv.getLong(field.c_str());

  if (ret < minv || ret > maxv) {
    std::stringstream msg;
    msg << "expected integer in range [" << minv << "," << maxv << "]";
    throw ParserException(msg.str(), field, csv.getCurLine());
  }

  return ret;
}

// ___________________________________________________________________________
int64_t Parser::getRangeInteger(const CsvParser& csv, const std::string& field,
                                int64_t minv, int64_t maxv, int64_t def) const {
  int64_t ret;

  if (csv.hasItem(field.c_str()) && !csv.fieldIsEmpty(field.c_str())) {
    ret = csv.getLong(field.c_str());

    if (ret < minv || ret > maxv) {
      std::stringstream msg;
      msg << "expected integer in range [" << minv << "," << maxv << "]";
      throw ParserException(msg.str(), field, csv.getCurLine());
    }

    return ret;
  }

  return def;
}

// ___________________________________________________________________________
uint32_t Parser::getColorFromHexString(const CsvParser& csv,
                                       const std::string& field,
                                       const std::string& def) const {
  std::string color_string;

  if (csv.hasItem(field.c_str())) {
    color_string = csv.getTString(field.c_str());
  }

  if (color_string.empty()) color_string = def;

  size_t chars_processed = 0;
  uint32_t ret = 0;

  try {
    ret = std::stoul("0x" + color_string, &chars_processed, 16);
  } catch (std::exception e) {
    std::stringstream msg;
    msg << "expected a 6-character hexadecimal color string, found '"
        << color_string << "' instead. (Error while parsing was: " << e.what()
        << ")";
    throw ParserException(msg.str(), field, csv.getCurLine());
  }

  if (color_string.size() != 6 || chars_processed != 8) {
    std::stringstream msg;
    msg << "expected a 6-character hexadecimal color string, found '"
        << color_string << "' instead.";
    throw ParserException(msg.str(), field, csv.getCurLine());
  }

  return ret;
}

// ____________________________________________________________________________
ServiceDate Parser::getServiceDate(const CsvParser& csv,
                                   const std::string& field) const {
  size_t p;
  std::string val(csv.getTString(field.c_str()));

  try {
    int32_t yyyymmdd = std::stoul(val, &p, 10);
    if (p != val.length() || yyyymmdd > 99999999) {
      std::stringstream msg;
      msg << "expected a date in the YYYYMMDD format, found '" << val
          << "' instead.";
      throw ParserException(msg.str(), field, csv.getCurLine());
    }
    return ServiceDate(yyyymmdd);
  } catch (const std::out_of_range& e) {
    std::stringstream msg;
    msg << "expected a date in the YYYYMMDD format, found '" << val
        << "' instead. (Integer out of range).";
    throw ParserException(msg.str(), field, csv.getCurLine());
  } catch (const std::invalid_argument& e) {
    std::stringstream msg;
    msg << "expected a date in the YYYYMMDD format, found '" << val
        << "' instead.";
    throw ParserException(msg.str(), field, csv.getCurLine());
  }
}

// ____________________________________________________________________________
Time Parser::getTime(const CsvParser& csv, const std::string& field) const {
  size_t p;
  std::string val(csv.getTString(field.c_str()));

  // TODO(patrick): null value
  if (val == "") return Time(0, 0, 0);

  try {
    uint64_t h = std::stoul(val, &p, 10);
    if (h > 255)
      throw std::out_of_range(
          "only hour-values up to 255 are "
          "supported. (read " +
          std::to_string(h) + ")");
    val.erase(0, p + 1);

    uint64_t m = std::stoul(val, &p, 10);
    if (p == 1)
      throw std::invalid_argument("one-digit minute values are not allowed.");
    // allow values of 60, although standard forbids it
    if (m > 60)
      throw std::out_of_range(
          "only minute-values up to 60 are "
          "allowed. (read " +
          std::to_string(m) + ")");
    val.erase(0, p + 1);

    uint64_t s = std::stoul(val, &p, 10);
    if (p == 0) s = 0;  // support HH:MM format (although standard forbids it)
    if (p == 1)
      throw std::invalid_argument("one-digit second values are not allowed.");
    // allow values of 60, although standard forbids it
    if (s > 60)
      throw std::out_of_range(
          "only second-values up to 60 are "
          "allowed. (read " +
          std::to_string(s) + ")");

    return Time(h, m % 60, s % 60);
  } catch (const std::exception& e) {
    std::stringstream msg;
    msg << "expected a time in HH:MM:SS (or H:MM:SS) format, found '" << val
        << "' instead. (" << e.what() << ")";
    throw ParserException(msg.str(), field, csv.getCurLine());
  }
}

// ____________________________________________________________________________
Route::TYPE Parser::getRouteType(const CsvParser& csv, const std::string& field,
                                 int64_t t) const {
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
    case 405:
      return Route::TYPE::RAIL;
    case 3:
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
    case 716:
    case 800:
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
    // TODO(patrick): from here on not complete!
    case 4:
    case 1000:
      return Route::TYPE::FERRY;
    case 6:
    case 1300:
      return Route::TYPE::GONDOLA;
    case 7:
    case 116:
    case 1400:
      return Route::TYPE::FUNICULAR;
    case 5:
    case 1500:
      return Route::TYPE::CABLE_CAR;
    default:
      std::stringstream msg;
      msg << "route type '" << t << " not supported.";
      throw ParserException(msg.str(), field, csv.getCurLine());
  }
}
