#include "json_reader.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"

#include <iostream>

using namespace std;

int main() {
    transport::TransportCatalogue catalogue;
    
    JsonReader reader(cin);

    reader.ProcessBaseRequests(catalogue);

    const auto render_settings = reader.ParseRenderSettings();
    renderer::MapRenderer map_renderer(render_settings);

    RequestHandler handler(catalogue, map_renderer);

    reader.ProcessStatRequests(handler, cout);

    return 0;
}
