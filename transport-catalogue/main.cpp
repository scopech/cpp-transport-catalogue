#include "json_reader.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <iostream>

using namespace std;

int main() {
    transport::TransportCatalogue catalogue;
    
    JsonReader reader(cin);

    reader.ProcessBaseRequests(catalogue);

    const auto render_settings = reader.ParseRenderSettings();
    renderer::MapRenderer map_renderer(render_settings);
    
    const auto routing_settings = reader.ParseRoutingSettings();
    transport::TransportRouter router(catalogue, routing_settings);

    RequestHandler handler(catalogue, map_renderer, router);

    reader.ProcessStatRequests(handler, cout);

    return 0;
}
