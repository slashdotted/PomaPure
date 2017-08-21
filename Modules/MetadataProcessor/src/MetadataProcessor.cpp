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

#include "MetadataProcessor.h"
#include <boost/algorithm/string.hpp>
#include <boost/io/detail/quoted_manip.hpp>
#include <iterator>
#include <fstream>
#include <boost/lexical_cast.hpp>
#include <functional>
#include <array>


DEFAULT_EXPORT_ALL(MetadataProcessor, "Processes values in the property tree", "", false)

MetadataProcessor::MetadataProcessor(const std::string& mid) : poma::Module<MetadataProcessor, PomaDataType>(mid) {}

void MetadataProcessor::setup_cli(boost::program_options::options_description& desc) const
{
    boost::program_options::options_description options("JSON Processor");
    options.add_options()
    ("code", boost::program_options::value<std::string>()->required(), "Code to be executed");
    desc.add(options);
}

void MetadataProcessor::process_cli(boost::program_options::variables_map& vm)
{
    std::ifstream script;
    script.open(vm["code"].as<std::string>());
    if (!script.fail()) {
        std::stringstream buffer;
        buffer << script.rdbuf();
        m_code = buffer.str();
        script.close();
    } else {
        std::cerr << "Error reading processor script" << std::endl;
        exit(1);
    }
}

void MetadataProcessor::initialize()
{
    try {
        parse();
    } catch (std::string e) {
        std::cerr << "Exception " << e << std::endl;
        exit(1);
    }

}

void MetadataProcessor::on_incoming_data(PomaPacketType& dta, const std::string& channel)
{
    try {
        execute(dta.m_properties);
    } catch (std::string e) {
        std::cerr << "Exception " << e << std::endl;
    }
    submit_data(dta);
}


void MetadataProcessor::parse()
{
    std::stringstream iss;
    iss << m_code;
    std::string i;
    std::string data;
    char ch;
    std::vector<std::string> def_labels;
    std::vector<std::string> jmp_labels;
    while(iss) {
        i = "end";
        iss >> i;
        boost::algorithm::to_lower(i);
        std::string num_arg[] = {"add", "sub", "div", "mul" };
        std::string no_arg[] = {"end", "dup", "pop", "imode", "dmode", "smode", "bmode", "lmode", "ordie" };
        if (std::find(std::begin(num_arg), std::end(num_arg), i) != std::end(num_arg)) {
            int value;
            iss >> value;
            if (i == "add") {
                m_instructions.push_back(JPInstruction{OpCode::ADD, value, "", i});
            } else if (i == "sub") {
                m_instructions.push_back(JPInstruction{OpCode::SUB, value, "", i});
            } else if (i == "div") {
                m_instructions.push_back(JPInstruction{OpCode::DIV, value, "", i});
            } else if (i == "mul") {
                m_instructions.push_back(JPInstruction{OpCode::MUL, value, "", i});
            } else {
                throw i;
            }
        } else if (std::find(std::begin(no_arg), std::end(no_arg), i) != std::end(no_arg)) {
            if (i == "end") {
                m_instructions.push_back(JPInstruction{OpCode::END, -1, "", i});
            } else if (i == "dup") {
                m_instructions.push_back(JPInstruction{OpCode::DUP, -1, "", i});
            } else if (i == "pop") {
                m_instructions.push_back(JPInstruction{OpCode::POP, -1, "", i});
            } else if (i == "imode") {
                m_instructions.push_back(JPInstruction{OpCode::IMODE, -1, "", i});
            } else if (i == "smode") {
                m_instructions.push_back(JPInstruction{OpCode::SMODE, -1, "", i});
            } else if (i == "dmode") {
                m_instructions.push_back(JPInstruction{OpCode::DMODE, -1, "", i});
            } else if (i == "bmode") {
                m_instructions.push_back(JPInstruction{OpCode::BMODE, -1, "", i});
            } else if (i == "lmode") {
                m_instructions.push_back(JPInstruction{OpCode::LMODE, -1, "", i});
            } else if (i == "ordie") {
                m_instructions.push_back(JPInstruction{OpCode::ORDIE, -1, "", i});
            } else {
                throw i;
            }
        } else {
            iss >> boost::io::quoted(data);
            if (i == "exists") {
                m_instructions.push_back(JPInstruction{OpCode::EXISTS, -1, data, i});
            } else if (i == "load") {
                m_instructions.push_back(JPInstruction{OpCode::LOAD, -1, data, i});
            } else if (i == "store") {
                m_instructions.push_back(JPInstruction{OpCode::STORE, -1, data, i});
            } else if (i == "jt") {
                jmp_labels.push_back(data);
                m_instructions.push_back(JPInstruction{OpCode::JT, -1, data, i});
            } else if (i == "jf") {
                jmp_labels.push_back(data);
                m_instructions.push_back(JPInstruction{OpCode::JF, -1, data, i});
            } else if (i == "jmp") {
                jmp_labels.push_back(data);
                m_instructions.push_back(JPInstruction{OpCode::JMP, -1, data, i});
            } else if (i == "eq") {
                m_instructions.push_back(JPInstruction{OpCode::EQ, -1, data, i});
            } else if (i == "ne") {
                m_instructions.push_back(JPInstruction{OpCode::NE, -1, data, i});
            } else if (i == "lt") {
                m_instructions.push_back(JPInstruction{OpCode::LT, -1, data, i});
            } else if (i == "le") {
                m_instructions.push_back(JPInstruction{OpCode::LE, -1, data, i});
            } else if (i == "gt") {
                m_instructions.push_back(JPInstruction{OpCode::GT, -1, data, i});
            } else if (i == "ge") {
                m_instructions.push_back(JPInstruction{OpCode::GE, -1, data, i});
            } else if (i == "push") {
                m_instructions.push_back(JPInstruction{OpCode::PUSH, -1, data, i});
            } else if (i == "label") {
                def_labels.push_back(data);
                m_instructions.push_back(JPInstruction{OpCode::LABEL, -1, data, i});
                m_jumptable[data] = m_instructions.size();
            } else if (i == "comment") {
                /* Do nothing */
            } else if (i == "") {
                /* Do nothing */
            } else {
                std::cout  << "Unknown operation " << i << std::endl;
                //throw i;
            }
        }
    }
    for (auto l : jmp_labels) {
        if (std::find(def_labels.begin(), def_labels.end(), l) == def_labels.end()) {
            throw std::string{"Undefined label"};
        }
    }
    m_instructions.push_back(JPInstruction{OpCode::END, -1, "", i});
}


template<typename Z>
bool MetadataProcessor::process(boost::property_tree::ptree& pt)
{
    for(;;) {
        if (m_pc+1 >= m_instructions.size()) {
            std::cerr << "Bad PC " << m_pc+1 << std::endl;
            exit(1);
        }
        JPInstruction& is{m_instructions[m_pc++]};
        //std::cout << "Exec:" << is.m_instruction << "- data:" << is.m_data <<  "- value:"<< is.m_value <<std::endl;
        try {
            switch(is.m_op) {
            case OpCode::EXISTS: {
                m_flag = (pt.get(is.m_data, is.m_data) != is.m_data);
                break;
            }
            case OpCode::LOAD: {
                m_stack.push_back(pt.get<Z>(is.m_data, [] { Z t; return t; }()));
                break;
            }
            case OpCode::PUSH: {
                std::stringstream ss{is.m_data};
                ss << is.m_data;
                Z value;
                ss >> value;
                m_stack.push_back(value);
                break;
            }
            case OpCode::POP: {
                m_stack.pop_back();
            }
            case OpCode::ADD: {
                Z tmp = 0;
                for (unsigned int i{0}; i<is.m_value; ++i) {
                    Z val{boost::get<Z>(m_stack.back())};
                    tmp += val;
                    m_stack.pop_back();
                }
                m_stack.push_back(tmp);
                break;
            }
            case OpCode::MUL: {
                Z tmp = 1;
                for (unsigned int i{0}; i<is.m_value; ++i) {
                    Z val{boost::get<Z>(m_stack.back())};
                    tmp *= val;
                    m_stack.pop_back();
                }
                m_stack.push_back(tmp);
                break;
            }
            case OpCode::DIV: {
                Z tmp{boost::get<Z>(m_stack.back())};
                m_stack.pop_back();
                for (unsigned int i{0}; i<is.m_value; ++i) {
                    Z val{boost::get<Z>(m_stack.back())};
                    tmp /= val;
                    m_stack.pop_back();
                }
                m_stack.push_back(tmp);
                break;
            }
            case OpCode::SUB: {
                Z tmp{boost::get<Z>(m_stack.back())};
                m_stack.pop_back();
                for (unsigned int i{0}; i<is.m_value; ++i) {
                    Z val{boost::get<Z>(m_stack.back())};
                    tmp -= val;
                    m_stack.pop_back();
                }
                m_stack.push_back(tmp);
                break;
            }
            case OpCode::STORE: {
                Z val{boost::get<Z>(m_stack.back())};
                m_stack.pop_back();
                pt.put(is.m_data, val);
                break;
            }
            case OpCode::IMODE:
                m_mode = Mode::INT;
                return true;
                break;
            case OpCode::DMODE:
                m_mode = Mode::DOUBLE;
                return true;
                break;
            case OpCode::SMODE:
                m_mode = Mode::STRING;
                return true;
                break;
            case OpCode::BMODE:
                m_mode = Mode::BOOL;
                return true;
                break;
            case OpCode::LMODE:
                m_mode = Mode::LONG;
                return true;
                break;
            case OpCode::JMP:
                m_pc = m_jumptable[is.m_data];
                break;
            case OpCode::EQ: {
                Z tos{boost::get<Z>(m_stack.back())};
                m_stack.pop_back();
                std::stringstream ss{is.m_data};
                ss << is.m_data;
                Z value;
                ss >> value;
                m_flag = (tos == value);
                break;
            }
            case OpCode::LT: {
                Z tos{boost::get<Z>(m_stack.back())};
                m_stack.pop_back();
                std::stringstream ss{is.m_data};
                ss << is.m_data;
                Z value;
                ss >> value;
                m_flag = (tos < value);
                break;
            }
            case OpCode::LE: {
                Z tos{boost::get<Z>(m_stack.back())};
                m_stack.pop_back();
                std::stringstream ss{is.m_data};
                ss << is.m_data;
                Z value;
                ss >> value;
                m_flag = (tos <= value);
                break;
            }
            case OpCode::GT: {
                Z tos{boost::get<Z>(m_stack.back())};
                m_stack.pop_back();
                std::stringstream ss{is.m_data};
                ss << is.m_data;
                Z value;
                ss >> value;
                m_flag = (tos > value);
                break;
            }
            case OpCode::GE: {
                Z tos{boost::get<Z>(m_stack.back())};
                m_stack.pop_back();
                std::stringstream ss{is.m_data};
                ss << is.m_data;
                Z value;
                ss >> value;
                m_flag = (tos >= value);
                break;
            }
            case OpCode::NE: {
                Z tos{boost::get<Z>(m_stack.back())};
                m_stack.pop_back();
                std::stringstream ss{is.m_data};
                ss << is.m_data;
                Z value;
                ss >> value;
                m_flag = (tos != value);
                break;
            }
            case OpCode::JT: {
                if (m_flag) {
                    m_pc = m_jumptable[is.m_data];
                }
                break;
            }
            case OpCode::JF: {
                if (!m_flag) {
                    m_pc = m_jumptable[is.m_data];
                }
                break;
            }
            case OpCode::DUP: {
                Z tos{boost::get<Z>(m_stack.back())};
                m_stack.push_back(tos);
                break;
            }
            case OpCode::ORDIE: {
                if (!m_flag) {
                    return false;
                }
                break;
            }
            case OpCode::LABEL:
                break;
            case OpCode::END:
                return false;
            default:
                std::cerr << "Invalid operation in numeric mode: " << is.m_instruction << std::endl;
                exit(1);
            }
        } catch(std::exception e) {
            std::cout << "Exception in numeric mode " << e.what() << " on Exec:" << is.m_instruction << "- data:" << is.m_data <<  "- value:"<< is.m_value <<std::endl;
            exit(1);
        }
    }
}


template<>
bool MetadataProcessor::process<std::string>(boost::property_tree::ptree& pt)
{
    for(;;) {
        if (m_pc+1 >= m_instructions.size()) {
            std::cerr << "Bad PC " << m_pc+1 << std::endl;
            exit(1);
        }
        JPInstruction& is{m_instructions[m_pc++]};
        try {
            switch(is.m_op) {
            case OpCode::EXISTS: {
                m_flag = (pt.get(is.m_data, is.m_data) != is.m_data);
                break;
            }
            case OpCode::LOAD: {
                m_stack.push_back(pt.get<std::string>(is.m_data, [] { std::string t; return t; }()));
                break;
            }
            case OpCode::PUSH: {
                m_stack.push_back(is.m_data);
                break;
            }
            case OpCode::POP: {
                m_stack.pop_back();
            }
            case OpCode::ADD: {
                std::string tmp;
                for (unsigned int i{0}; i<is.m_value; ++i) {
                    std::string val{boost::get<std::string>(m_stack.back())};
                    tmp += val;
                    m_stack.pop_back();
                }
                m_stack.push_back(tmp);
                break;
            }
            case OpCode::STORE: {
                std::string val{boost::get<std::string>(m_stack.back())};
                m_stack.pop_back();
                pt.put(is.m_data, val);
                break;
            }
            case OpCode::IMODE:
                m_mode = Mode::INT;
                return true;
                break;
            case OpCode::DMODE:
                m_mode = Mode::DOUBLE;
                return true;
                break;
            case OpCode::SMODE:
                m_mode = Mode::STRING;
                return true;
                break;
            case OpCode::BMODE:
                m_mode = Mode::BOOL;
                return true;
                break;
            case OpCode::LMODE:
                m_mode = Mode::LONG;
                return true;
                break;
            case OpCode::JMP:
                m_pc = m_jumptable[is.m_data];
                break;
            case OpCode::EQ: {
                std::string tos{boost::get<std::string>(m_stack.back())};
                m_stack.pop_back();
                m_flag = (tos == is.m_data);
                break;
            }
            case OpCode::NE: {
                std::string tos{boost::get<std::string>(m_stack.back())};
                m_stack.pop_back();
                m_flag = (tos != is.m_data);
                break;
            }
            case OpCode::JT: {
                if (m_flag) {
                    m_pc = m_jumptable[is.m_data];
                }
                break;
            }
            case OpCode::JF: {
                if (!m_flag) {
                    m_pc = m_jumptable[is.m_data];
                }
                break;
            }
            case OpCode::DUP: {
                std::string tos{boost::get<std::string>(m_stack.back())};
                m_stack.push_back(tos);
                break;
            }
            case OpCode::LABEL:
                break;
            case OpCode::ORDIE: {
                if (!m_flag) {
                    return false;
                }
                break;
            }
            case OpCode::END:
                return false;
            default:
                std::cerr << "Invalid operation in string mode: " << is.m_instruction << std::endl;
                exit(1);
            }
        } catch(std::exception e) {
            std::cout << "Exception in string mode " << e.what() << " on Exec:" << is.m_instruction << "- data:" << is.m_data <<  "- value:"<< is.m_value <<std::endl;
            exit(1);
        }
    }
}


template<>
bool MetadataProcessor::process<bool>(boost::property_tree::ptree& pt)
{
    for(;;) {
        if (m_pc+1 >= m_instructions.size()) {
            std::cerr << "Bad PC " << m_pc+1 << std::endl;
            exit(1);
        }
        JPInstruction& is{m_instructions[m_pc++]};
        try {
            switch(is.m_op) {
            case OpCode::EXISTS: {
                m_flag = (pt.get(is.m_data, is.m_data) != is.m_data);
                break;
            }
            case OpCode::LOAD: {
                m_stack.push_back(pt.get<bool>(is.m_data, [] { bool t; return t; }()));
                break;
            }
            case OpCode::PUSH: {
                m_stack.push_back(is.m_data);
                break;
            }
            case OpCode::POP: {
                m_stack.pop_back();
            }
            case OpCode::STORE: {
                bool val{boost::get<bool>(m_stack.back())};
                m_stack.pop_back();
                pt.put(is.m_data, val);
                break;
            }
            case OpCode::IMODE:
                m_mode = Mode::INT;
                return true;
                break;
            case OpCode::DMODE:
                m_mode = Mode::DOUBLE;
                return true;
                break;
            case OpCode::SMODE:
                m_mode = Mode::STRING;
                return true;
                break;
            case OpCode::BMODE:
                m_mode = Mode::BOOL;
                return true;
                break;
            case OpCode::LMODE:
                m_mode = Mode::LONG;
                return true;
                break;
            case OpCode::JMP:
                m_pc = m_jumptable[is.m_data];
                break;
            case OpCode::EQ: {
                bool tos{boost::get<bool>(m_stack.back())};
                m_stack.pop_back();
                std::stringstream ss{is.m_data};
                ss << is.m_data;
                bool value;
                ss >> value;
                m_flag = (tos == value);
                break;
            }
            case OpCode::NE: {
                bool tos{boost::get<bool>(m_stack.back())};
                m_stack.pop_back();
                std::stringstream ss{is.m_data};
                ss << is.m_data;
                bool value;
                ss >> value;
                m_flag = (tos != value);
                break;
            }
            case OpCode::JT: {
                if (m_flag) {
                    m_pc = m_jumptable[is.m_data];
                }
            }
            case OpCode::JF: {
                if (!m_flag) {
                    m_pc = m_jumptable[is.m_data];
                }
                break;
            }
            case OpCode::DUP: {
                bool tos{boost::get<bool>(m_stack.back())};
                m_stack.push_back(tos);
                break;
            }
            case OpCode::LABEL:
                break;
            case OpCode::ORDIE: {
                if (!m_flag) {
                    return false;
                }
                break;
            }
            case OpCode::END:
                return false;
            default:
                std::cerr << "Invalid operation in bool mode: " << is.m_instruction << std::endl;
                exit(1);
            }
        } catch(std::exception e) {
            std::cout << "Exception in boolean mode " << e.what() << " on Exec:" << is.m_instruction << "- data:" << is.m_data <<  "- value:"<< is.m_value <<std::endl;
            exit(1);
        }
    }
}

void MetadataProcessor::execute(boost::property_tree::ptree& pt)
{
    if (m_instructions.size() <= 0) return;
    m_pc = 0;
    for(;;) {
        switch(m_mode) {
        case Mode::DOUBLE:
            if (!process<double>(pt)) {
                return;
            }
            break;
        case Mode::INT:
            if (!process<int>(pt)) {
                return;
            }
            break;
        case Mode::LONG:
            if (!process<long>(pt)) {
                return;
            }
            break;
        case Mode::STRING:
            if (!process<std::string>(pt)) {
                return;
            }
            break;
        case Mode::BOOL:
            if (!process<bool>(pt)) {
                return;
            }
            break;
        }
    }
}





