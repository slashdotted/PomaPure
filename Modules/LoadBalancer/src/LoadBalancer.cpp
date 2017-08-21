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

#include "LoadBalancer.h"
#include <cstdlib>

DEFAULT_EXPORT_ALL(LoadBalancer, "Load balancer", "", false)

LoadBalancer::LoadBalancer(const std::string& mid) : poma::Module<LoadBalancer, PomaDataType>(mid) {}

void LoadBalancer::on_incoming_data(PomaPacketType& dta, const std::string& channel)
{
    if (m_sinks == 0) {
        submit_data(dta);
    } else {
        if (m_random) {
            m_index = std::rand() % m_sinks;
        } else {
            m_index = (m_index + 1) % m_sinks;
        }
        std::stringstream c;
        c << m_index;
        submit_data(dta, c.str());
    }
}

void LoadBalancer::setup_cli(boost::program_options::options_description& desc) const
{
    boost::program_options::options_description buf("LoadBalancer options");
    buf.add_options()
    ("random", boost::program_options::value<bool>()->default_value(false), "use random selection instead of round robin")
    ("sinks", boost::program_options::value<int>()->default_value(0), "number of sinks");
    desc.add(buf);
}

void LoadBalancer::process_cli(boost::program_options::variables_map& vm)
{
    m_sinks = vm["sinks"].as<int>();
    m_random = vm["random"].as<bool>();
}
