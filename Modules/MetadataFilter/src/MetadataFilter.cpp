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

#include "MetadataFilter.h"
#include <boost/lexical_cast.hpp>

DEFAULT_EXPORT_ALL(MetadataFilter, "Filter blobs depending on JSON values", "", false)

static bool compare_string_equal(std::string& a, std::string& b)
{
    return a == b;
}

static bool compare_string_not_equal(std::string& a, std::string& b)
{
    return a != b;
}

static bool compare_double_equal(double a, double b)
{
    return a == b;
}

static bool compare_double_not_equal(double a, double b)
{
    return a != b;
}

static bool compare_double_gt(double a, double b)
{
    return a > b;
}

static bool compare_double_ge(double a, double b)
{
    return a >= b;
}

static bool compare_double_lt(double a, double b)
{
    return a < b;
}

static bool compare_double_le(double a, double b)
{
    return a <= b;
}

MetadataFilter::MetadataFilter(const std::string& mid) : poma::Module<MetadataFilter, PomaDataType>(mid) {}

void MetadataFilter::setup_cli(boost::program_options::options_description& desc) const
{
    boost::program_options::options_description options("JSON Filter Capture Options");
    options.add_options()
    ("field", boost::program_options::value<std::string>()->required(), "JSON field to be compared")
    ("op", boost::program_options::value<std::string>()->required(), "Comparison operation (>,<,=,!=,<=,>=)")
    ("value", boost::program_options::value<std::string>()->required(), "Value to compare to");
    desc.add(options);
}

void MetadataFilter::process_cli(boost::program_options::variables_map& vm)
{
    m_field = vm["field"].as<std::string>();
    m_op = vm["op"].as<std::string>();
    m_value_string = vm["value"].as<std::string>();

    try {
        m_value_double = boost::lexical_cast<double>(m_value_string);
        m_string_comparison = false;
    } catch(...) {
        m_string_comparison = true;
    }

    if (m_string_comparison) {
        if (m_op == "=") {
            m_string_comparison_fn = compare_string_equal;
        } else if (m_op == "!=") {
            m_string_comparison_fn = compare_string_not_equal;
        } else {
            std::cerr << "Invalid string operator " << m_op << std::endl;
            exit(1);
        }
    } else {
        if (m_op == "=") {
            m_double_comparison_fn = compare_double_equal;
        } else if (m_op == "!=") {
            m_double_comparison_fn = compare_double_not_equal;
        } else if (m_op == "<") {
            m_double_comparison_fn = compare_double_lt;
        } else if (m_op == "<=") {
            m_double_comparison_fn = compare_double_le;
        } else if (m_op == ">") {
            m_double_comparison_fn = compare_double_gt;
        } else if (m_op == ">=") {
            m_double_comparison_fn = compare_double_ge;
        } else {
            std::cerr << "Invalid double operator " << m_op << std::endl;
            exit(1);
        }
    }
}

void MetadataFilter::on_incoming_data(PomaPacketType& dta, const std::string& channel)
{
    if (m_string_comparison) {
        std::string data = dta.m_properties.get(m_field, "");
        if (!m_string_comparison_fn(data, m_value_string)) {
            submit_data(dta, "fail");
        } else {
            submit_data(dta);
        }
    } else {
        double data = dta.m_properties.get(m_field, -1.0);
        if (!m_double_comparison_fn(data, m_value_double)) {
            submit_data(dta, "fail");
        } else {
            submit_data(dta);
        }
    }
}
