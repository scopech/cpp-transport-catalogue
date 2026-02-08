#include "stat_reader.h"
#include <iostream>
#include <iomanip>

namespace transport::stat {

namespace detail {
    
    std::string_view TrimRequest(std::string_view str) {
        const auto start = str.find_first_not_of(' ');
        if (start == std::string_view::npos) {
            return {};
        }
        return str.substr(start, str.find_last_not_of(' ') + 1 - start);
    }

} // namespace detail

void ParseAndPrintStat(const transport::TransportCatalogue& transport_catalogue, 
                       std::string_view request,
                       std::ostream& output) {
    using namespace detail;

    std::string_view command_bus = "Bus ";
    std::string_view command_stop = "Stop ";

    if (request.substr(0, command_bus.size()) == command_bus) {
        std::string_view bus_name = TrimRequest(request.substr(command_bus.size()));
        auto stat = transport_catalogue.GetBusInfo(bus_name);

        if (stat) {
            output << "Bus " << bus_name << ": " 
                   << stat->stops_count << " stops on route, " 
                   << stat->unique_stops_count << " unique stops, " 
                   << std::setprecision(6) << stat->route_length << " route length, "
                   << std::setprecision(6) << stat->curvature << " curvature" << std::endl;
        } else {
            output << "Bus " << bus_name << ": not found" << std::endl;
        }

    } else if (request.substr(0, command_stop.size()) == command_stop) {
        std::string_view stop_name = TrimRequest(request.substr(command_stop.size()));
        auto stat = transport_catalogue.GetStopInfo(stop_name);

        if (stat) {
            const auto& buses = *stat->buses;
            if (buses.empty()) {
                output << "Stop " << stop_name << ": no buses" << std::endl;
            } else {
                output << "Stop " << stop_name << ": buses";
                for (std::string_view bus : buses) {
                    output << " " << bus;
                }
                output << std::endl;
            }
        } else {
            output << "Stop " << stop_name << ": not found" << std::endl;
        }
    }
}

} // namespace transport::stat
