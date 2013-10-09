/*
   Copyright 2013 Roman Kashitsyn

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "config/ini/parser.hpp"
#include <sstream>
#include <cctype>

namespace config { namespace ini {

namespace {
const char * event_type_to_string(parser::event_type t)
{
    switch (t) {
    case parser::EVENT_ERROR: return "ERROR";
    case parser::EVENT_SECTION: return "SECTION";
    case parser::EVENT_NAME: return "NAME";
    case parser::EVENT_VALUE: return "VALUE";
    case parser::EVENT_END: return "END";
    default: return "UNKNOWN";
    }
}
}

parser::parser(std::istream & is)
    : in_(is)
    , state_(&parser::advance_gen)
{}

bool parser::advance(event & e)
{
    return (this->*state_)(e);
}

char parser::get_char()
{
    ++pos_;
    return in_.get();
}

void parser::put_back(char c)
{
    --pos_;
    in_.putback(c);
}

bool parser::advance_gen(event & e)
{
    for (;;) {
        const char c = get_char();
        if (handle_eof(e)) return false;
        switch (c) {
        case '\r':
            check_lf();
        case '\n':
            handle_new_line();
            break;
        case ';':
            return advance_comment(e);
        case '[':
            return advance_section(e);
        default:
            if (std::isspace(c)) continue;
            if (std::isalnum(c)) {
                put_back(c);
                return advance_param(e);
            }
            std::ostringstream ss(e.value);
            ss << "Unexpected symbol " << c
               << " at [" << line_ << "," << pos_ << "]";
            e.type = EVENT_ERROR;
            return false;
        }
    }
}

bool parser::advance_comment(event & e)
{
    for (;;) {
        const char c = get_char();
        switch (c) {
        case '\r':
            check_lf();
        case '\n':
            handle_new_line();
            return advance_gen(e);
        default:
            if (handle_eof(e)) return false;
            /* Consuming symbols till the end of the string */
            continue;
        }
    }
}

bool parser::advance_section(event & e)
{
    e.value.clear();
    for (;;) {
        const char c = get_char();
        if (handle_eof(e)) {
            unexpected_token(e, "end of file");
            return false;
        }
        switch (c) {
        case ';':
            unexpected_token(e, "comment");
            return false;
        case '\r':
        case '\n':
            unexpected_token(e, "end of line");
            return false;
        case ']':
            if (e.value.empty()) {
                e.type = EVENT_ERROR;
            } else {
                e.type = EVENT_SECTION;
            }
            state_ = &parser::advance_gen;
            return true;
        default:
            e.value.push_back(c);
        }
    }
}

bool parser::advance_param(event & e)
{
    e.value.clear();
    for (;;) {
        const char c = get_char();

        if (in_.eof()) {
            state_ = &parser::advance_eof;
            unexpected_token(e, "end of line");
            return false;
        }

        switch (c) {
        case ';':
            unexpected_token(e, "comment");
            return false;
        case '\r':
            check_lf();
        case '\n':
            unexpected_token(e, "new line");
            return false;
        case '=':
            state_ = &parser::advance_value;
            e.type = EVENT_NAME;
            return true;
        default:
            e.value.push_back(c);
        }
    }
}

bool parser::advance_value(event & e)
{
    e.value.clear();
    for (;;) {
        const char c = get_char();

        if (in_.eof()) {
            state_ = &parser::advance_eof;
            const bool empty = e.value.empty();
            if (empty) {
                unexpected_token(e, "end of file");
            } else {
                e.type = EVENT_VALUE;
            }
            return !empty;
        }

        switch (c) {
        case '\r':
            check_lf();
        case '\n':
            handle_new_line();
            state_ = &parser::advance_gen;
            e.type = EVENT_VALUE;
            return true;
        default:
            e.value.push_back(c);
        }
    }
}

bool parser::handle_eof(event & e)
{
    const bool eof = in_.eof();
    if (eof) advance_eof(e);
    return eof;
}

bool parser::advance_eof(event & e)
{
    state_ = &parser::advance_eof;
    e.type = EVENT_END;
    e.value.clear();
    return false;
}

void parser::check_lf()
{
    const char c = get_char();
    if (c != '\n') put_back(c);
}

void parser::handle_new_line()
{
    pos_ = 0;
    ++line_;
}

void parser::unexpected_token(event & e, const char *desc)
{
    std::ostringstream ss(e.value);
    ss << "Unexpected " << desc
       << " at [" << line_ << ',' << pos_ << ']';
    e.type = EVENT_ERROR;
}

bool operator==(const parser::event & lhs,
                const parser::event & rhs)
{
    return lhs.type == rhs.type && lhs.value == rhs.value;
}

std::ostream & operator<<(std::ostream & os, const parser::event & e)
{
    os << "event{" << event_type_to_string(e.type)
       << ", \"" << e.value << "\"}";
}

} }
