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

#include "Stats.h"
#include <chrono>

DEFAULT_EXPORT_ALL(Stats, "Write processing stats", "", false)

using namespace std::chrono;

Stats::Stats(const std::string& mid) : poma::Module<Stats, PomaDataType>(mid) {}

void Stats::on_incoming_data(PomaPacketType& dta, const std::string& channel)
{
    m_packets++;
    auto before = high_resolution_clock::now();
    submit_data(dta);
    auto now = high_resolution_clock::now();
    m_elapsed_total += duration_cast<milliseconds>(now - before).count();
    if (m_packets % m_stats_interval == 0) {
        double pps = (double) m_packets / (m_elapsed_total / 1000.0);
        std::cout << ">>>> (" << m_module_id << ") " << m_packets << " packets processed, average processing time: " << m_elapsed_total / m_packets << " msec/frame, " << pps << " pps" << std::endl;
        m_packets = 0;
        m_elapsed_total = 0;
    }
}

void Stats::setup_cli(boost::program_options::options_description& desc) const
{
    boost::program_options::options_description buf("Stats options");
    buf.add_options()
    ("statsinterval", boost::program_options::value<int>()->default_value(256), "interval (packets) between stats printout");
    desc.add(buf);
}

void Stats::process_cli(boost::program_options::variables_map& vm)
{
    m_stats_interval = vm["statsinterval"].as<int>();
}


