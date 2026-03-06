#include "json_reader.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"

#include <iostream>

using namespace std;

int main() {
    transport::TransportCatalogue catalogue;
    JsonReader json_reader(cin);
    
    json_reader.ProcessBaseRequests(catalogue);
    
    renderer::RenderSettings render_settings;
    if (json_reader.HasRenderSettings()) {
        render_settings = json_reader.ParseRenderSettings();
    }
    
    renderer::MapRenderer renderer(render_settings);
    RequestHandler handler(catalogue, renderer);
    
    json_reader.ProcessStatRequests(handler, cout);
    
    return 0;
}
