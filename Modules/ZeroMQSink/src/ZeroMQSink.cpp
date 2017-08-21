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

#include "ZeroMQSink.h"

DEFAULT_EXPORT_ALL(ZeroMQSink, "ZeroMQ Sink module", "", false)

ZeroMQSink::ZeroMQSink(const std::string& mid) : poma::Module<ZeroMQSink, PomaDataType>(mid) {}

ZeroMQSink::ZeroMQSink(const ZeroMQSink& o) : poma::Module<ZeroMQSink, PomaDataType>(o.m_module_id), m_sink_address {o.m_sink_address}
{
    if (o.m_socket != nullptr) {
        initialize();
    }
}

ZeroMQSink& ZeroMQSink::operator=(const ZeroMQSink& o)
{
    if (m_socket != nullptr) {
        delete m_socket;
    }
    m_sink_address = o.m_sink_address;
    initialize();
    return *this;
}

ZeroMQSink::~ZeroMQSink()
{
    if (m_socket != nullptr) {
        delete m_socket;
    }
}

void ZeroMQSink::setup_cli(boost::program_options::options_description& desc) const
{
    boost::program_options::options_description ZeroMQSink("0MQ sink options");
    ZeroMQSink.add_options()
    ("sinkaddress", boost::program_options::value<std::string>()->default_value("tcp://localhost:7467"), "0MQ sink socket address");
    desc.add(ZeroMQSink);
}

void ZeroMQSink::process_cli(boost::program_options::variables_map& vm)
{
    m_sink_address = vm["sinkaddress"].as<std::string>();
}

void ZeroMQSink::initialize()
{
    m_socket = new zmq::socket_t {m_context, ZMQ_REQ};
    m_socket->connect(m_sink_address.c_str());
}

void ZeroMQSink::on_incoming_data(PomaPacketType& dta, const std::string& channel)
{
    dta.m_properties.put("zeromq.channel", channel);
    std::string data;
    serialize(dta, data);
    s_send(*m_socket, data);
    std::string ack{s_recv(*m_socket)};
    assert(ack == "ACK");
    submit_data(dta);
}
