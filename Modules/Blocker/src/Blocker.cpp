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

#include "Blocker.h"
#include <chrono>
#include <thread>

DEFAULT_EXPORT_ALL(Blocker, "Blocker module (delays packets for the specified amount of time)", "", false)

Blocker::Blocker(const std::string& mid) : Module<Blocker, PomaDataType>(mid)
{
}

void Blocker::on_incoming_data(PomaPacketType& dta, const std::string& channel)
{
    if (m_wait < 0) {
        for(;;) {};
    } else if (m_wait > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(m_wait));
    }
    submit_data(dta);
}

void Blocker::setup_cli(boost::program_options::options_description& desc) const
{
    boost::program_options::options_description buf("Blocker options");
    buf.add_options()
    ("wait", boost::program_options::value<int>()->default_value(1), "wait time in seconds (-1 infinite)");
    desc.add(buf);
}

void Blocker::process_cli(boost::program_options::variables_map& vm)
{
    m_wait = vm["wait"].as<int>();
}
