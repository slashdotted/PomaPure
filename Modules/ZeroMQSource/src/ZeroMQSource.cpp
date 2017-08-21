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

#include "ZeroMQSource.h"

DEFAULT_EXPORT_ALL(ZeroMQSource, "ZeroMQ Source module", "", false)

ZeroMQSource::ZeroMQSource(const std::string& mid) : poma::Module<ZeroMQSource, PomaDataType>(mid) {}

ZeroMQSource::ZeroMQSource(const ZeroMQSource& o) : poma::Module<ZeroMQSource, PomaDataType>(o.m_module_id), m_source_address {o.m_source_address}
{
    if (o.m_socket != nullptr) {
        initialize();
    }
}

ZeroMQSource& ZeroMQSource::operator=(const ZeroMQSource& o)
{
    if (m_socket != nullptr) {
        delete m_socket;
    }
    m_source_address = o.m_source_address;
    initialize();
    return *this;
}

ZeroMQSource::~ZeroMQSource()
{
    if (m_socket != nullptr) {
        delete m_socket;
    }
    if (m_thread != nullptr) {
        m_thread->detach();
    }
}

void ZeroMQSource::setup_cli(boost::program_options::options_description& desc) const
{
    boost::program_options::options_description ZeroMQSource("0MQ source options");
    ZeroMQSource.add_options()
    ("sourceaddress", boost::program_options::value<std::string>()->default_value("tcp://*:7467"), "0MQ source socket address");
    desc.add(ZeroMQSource);
}

void ZeroMQSource::process_cli(boost::program_options::variables_map& vm)
{
    m_source_address = vm["sourceaddress"].as<std::string>();
}

void ZeroMQSource::initialize()
{
    m_socket = new zmq::socket_t {m_context, ZMQ_REP};
    m_socket->bind(m_source_address.c_str());
    m_thread = new std::thread {&ZeroMQSource::serve_fn, this};
}

void ZeroMQSource::start_processing()
{
    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
}

void ZeroMQSource::serve_fn()
{
    for (;;) {
        std::string data_string{s_recv(*m_socket)};
        try {
            PomaPacketType dta;
            deserialize(dta, data_string);
            std::string channel{dta.m_properties.get("zeromq.channel", "default")};
            s_send(*m_socket, "ACK");
            submit_data(dta, channel);
        } catch (...) {

        }
    }
}
