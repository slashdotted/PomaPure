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

#ifndef METADATAPROCESSOR_H
#define METADATAPROCESSOR_H

#include "PomaDefault.h"
#include <unordered_map>
#include <vector>
#include <boost/variant.hpp>

class MetadataProcessor : public poma::Module<MetadataProcessor, PomaDataType> {
public:
    MetadataProcessor(const std::string& mid);

    void setup_cli(boost::program_options::options_description& desc) const override;
    void process_cli(boost::program_options::variables_map& vm) override;
    void initialize() override;
    void on_incoming_data(PomaPacketType& dta, const std::string& channel);

private:

    typedef boost::variant<int, long, double, std::string, bool> StackValue;

    enum class Mode {
        DOUBLE, INT, LONG, STRING, BOOL
    };

    enum class OpCode {
        /* Instruction with string data */
        EXISTS, LOAD, PUSH, STORE, JT, JF, JMP, EQ, LT, LE, GT, GE, NE, LABEL, COMMENT,
        /* Instructions with numerical data */
        ADD, MUL, DIV, SUB,
        /* Instructions with no data */
        END, DUP, POP, IMODE, DMODE, SMODE, BMODE, LMODE, ORDIE
    };

    struct JPInstruction {
        JPInstruction(OpCode op, int value, std::string data, std::string instruction)
            : m_op{op}, m_value{value}, m_data{data}, m_instruction{instruction} {}
        OpCode m_op;
        int m_value;
        std::string m_data;
        std::string m_instruction;
    };

    void execute(boost::property_tree::ptree& pt);
    template<typename Z> bool process(boost::property_tree::ptree& pt);
    void parse();

    std::unordered_map<std::string,int> m_jumptable;
    std::string m_code;
    bool m_flag{false};
    Mode m_mode{Mode::DOUBLE};
    unsigned int m_pc{0};
    std::vector<JPInstruction> m_instructions;
    std::vector<StackValue> m_stack;
};


#endif
