/*
 * Copyright (C)2015,2016,2017 Amos Brocco (amos.brocco@supsi.ch)
 *                             Scuola Universitaria Professionale della
 *                             Svizzera Italiana (SUPSI)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Scuola Universitaria Professionale della Svizzera
 *       Italiana (SUPSI) nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without
 *       specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <iostream>
#include <boost/property_tree/json_parser.hpp> // for write_json
#include "PomaLoader.h"

using namespace poma;

/* 1. We define a "local" sink module to collect results at the end of the pipeline
 *    This is necessary because the CallbackSinkModule is a template class that
 *    knowns nothing about the actual datatype exchanged by modules */
class LocalSink : public CallbackSinkModule<PomaDataType> {
public:
    LocalSink(std::string mid) : CallbackSinkModule<PomaDataType>(mid) {}
};

/* 2. We define the callback function that will be called by the end sink */
void my_callback (PomaPacketType& dta, const std::string& channel)
{
    std::stringstream swriter;
    boost::property_tree::write_json(swriter, dta.m_properties);
    std::cout << "Received data from channel " << channel << ": " << swriter.str() << std::endl;
}

int main(int argc, char* argv[])
{
    /* 3. We define pipeline (we could also load a pipeline from a file) */
    std::stringstream pipeline;
    pipeline << "{ "
             << "   \"source\" : \"dummy1\","
             << "   \"modules\" : {"
             << "		\"dummy1\" : { \"type\" : \"Dummy\" },"
             << "		\"dummy2\" : { \"type\" : \"Dummy\" },"
             << "		\"dummy3\" : { \"type\" : \"Dummy\" }"
             << "   }, "
             << "   \"links\" : ["
             << "		{ \"from\" : \"dummy1\", \"to\" : \"dummy2\" },"
             << "		{ \"from\" : \"dummy2\", \"to\" : \"dummy3\" }"
             << "   ]"
             << "}";

    /* 4. We create a loader object and let it enumerate our libraries */
    Loader loader;
    loader.enumerate_libraries("Modules");

    /* 5. We load the pipeline */
    loader.parse_json_config(pipeline);

    /* 6. We setup the local sink module, set the callback function and
     *    connect the module as the new pipeline sink */
    auto pipeline_sink{loader.get_module("dummy3")};
    auto ls = std::make_shared<LocalSink>("localsink");
    ls->set_callback(my_callback);
    pipeline_sink->connect_sink(ls);

    /* 7. We obtain the pipeline source */
    auto pipeline_source{loader.get_source()};

    /* 8. We initialize all modules */
    loader.initialize();

    /* 9. We start pushing packets to the pipeline */
    for (int i{0}; i<10000; i++) {
        PomaPacketType du;
        du.m_properties.put("sample.text", "Example");
        du.m_properties.put("sample.data", i);
        pipeline_source->on_incoming_data(du, "default");
    }

    /* 10. Before terminating, we flush the pipeline */
    loader.flush();
}

