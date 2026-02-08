#include "input_reader.h"
#include "geo.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>

namespace transport::input {

namespace detail {

    struct StopDescription {
        geo::Coordinates coordinates;
        std::vector<std::pair<std::string, double>> distances;
    };

    StopDescription ParseStopDescription(std::string_view description) {
        StopDescription result;
        
        size_t comma1 = description.find(',');
        if (comma1 == std::string_view::npos) {
            return result;
        }
        
        std::string lat_str(description.substr(0, comma1));
        size_t comma2 = description.find(',', comma1 + 1);
        if (comma2 == std::string_view::npos) {
            std::string lng_str(description.substr(comma1 + 1));
            result.coordinates = {std::stod(lat_str), std::stod(lng_str)};
            return result;
        }
        
        std::string lng_str(description.substr(comma1 + 1, comma2 - comma1 - 1));
        result.coordinates = {std::stod(lat_str), std::stod(lng_str)};
        
        std::string_view distances_str = description.substr(comma2 + 1);
        size_t pos = 0;
        
        while (pos < distances_str.length()) {
            while (pos < distances_str.length() && distances_str[pos] == ' ') ++pos;
            if (pos >= distances_str.length()) break;
            
            size_t m_pos = distances_str.find('m', pos);
            if (m_pos == std::string_view::npos) break;
            
            std::string dist_str(distances_str.substr(pos, m_pos - pos));
            double distance = std::stod(dist_str);
            
            pos = m_pos + 1;
            while (pos < distances_str.length() && distances_str[pos] == ' ') ++pos;
            
            if (pos + 3 >= distances_str.length() || 
                distances_str.substr(pos, 3) != "to ") {
                break;
            }
            pos += 3;
            
            size_t next_comma = distances_str.find(',', pos);
            std::string stop_name;
            if (next_comma == std::string_view::npos) {
                stop_name = std::string(distances_str.substr(pos));
                pos = distances_str.length();
            } else {
                stop_name = std::string(distances_str.substr(pos, next_comma - pos));
                pos = next_comma + 1;
            }
            
            size_t last_non_space = stop_name.find_last_not_of(' ');
            if (last_non_space != std::string::npos) {
                stop_name = stop_name.substr(0, last_non_space + 1);
            }
            
            result.distances.emplace_back(std::move(stop_name), distance);
        }
        
        return result;
    }

    std::string_view Trim(std::string_view string) {
        const auto start = string.find_first_not_of(' ');
        if (start == std::string_view::npos) {
            return {};
        }
        return string.substr(start, string.find_last_not_of(' ') + 1 - start);
    }

    std::vector<std::string_view> Split(std::string_view string, char delim) {
        std::vector<std::string_view> result;
        size_t pos = 0;
        while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
            auto delim_pos = string.find(delim, pos);
            if (delim_pos == std::string_view::npos) {
                delim_pos = string.size();
            }
            if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
                result.push_back(substr);
            }
            pos = delim_pos + 1;
        }
        return result;
    }

    std::vector<std::string_view> ParseRoute(std::string_view route) {
        if (route.find('>') != std::string_view::npos) {
            return Split(route, '>');
        }
        auto stops = Split(route, '-');
        std::vector<std::string_view> results(stops.begin(), stops.end());
        results.insert(results.end(), std::next(stops.rbegin()), stops.rend());
        return results;
    }

    CommandDescription ParseCommandDescription(std::string_view line) {
        auto colon_pos = line.find(':');
        if (colon_pos == std::string_view::npos) {
            return {};
        }
        auto space_pos = line.find(' ');
        if (space_pos >= colon_pos) {
            return {};
        }
        auto not_space = line.find_first_not_of(' ', space_pos);
        if (not_space >= colon_pos) {
            return {};
        }
        return {std::string(line.substr(0, space_pos)),
                std::string(line.substr(not_space, colon_pos - not_space)),
                std::string(line.substr(colon_pos + 1))};
    }

} // namespace detail

void InputReader::ParseLine(std::string_view line) {
    auto command_description = detail::ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands(transport::TransportCatalogue& catalogue) const {
    std::vector<std::pair<domain::Stop*, detail::StopDescription>> stop_descriptions;
    
    for (const auto& command : commands_) {
        if (command.command == "Stop") {
            auto desc = detail::ParseStopDescription(command.description);
            catalogue.AddStop(command.id, desc.coordinates);
            auto stop_ptr = const_cast<domain::Stop*>(catalogue.FindStop(command.id));
            stop_descriptions.emplace_back(stop_ptr, std::move(desc));
        }
    }
    
    for (const auto& [stop_ptr, desc] : stop_descriptions) {
        for (const auto& [to_stop_name, distance] : desc.distances) {
            auto to_stop = catalogue.FindStop(to_stop_name);
            if (stop_ptr && to_stop) {
                catalogue.SetDistance(stop_ptr, to_stop, distance);
            }
        }
    }
    
    for (const auto& command : commands_) {
        if (command.command == "Bus") {
            catalogue.AddBus(command.id, detail::ParseRoute(command.description));
        }
    }
}

} // namespace transport::input
