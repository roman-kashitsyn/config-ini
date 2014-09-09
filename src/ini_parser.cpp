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

/**
 * @file
 *
 * @detail The parser is implemented as finite state machine. Each
 * state is represented as member function pointer. When client calls
 * the advance() function the parser produces an event and switches
 * it's parse function to handle next expected event.
 */

#include "config/ini/parser.hpp"
#include <sstream>
#include <cctype>

namespace config {
namespace ini {

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

void trim_right(std::string & s)
{
    const std::size_t n = s.length();
    std::size_t i = n;
    while (i && std::isspace(s[i - 1])) --i;
    if (i != n) {
        s.erase(i);
    }
}
}

parser::parser(const std::string &filename, std::istream &is)
    : in_(is)
    , filename_(filename)
    , state_(&parser::advance_gen)
    , line_(0)
    , pos_(0)
{}

parser::parser(std::istream & is)
    : in_(is)
    , filename_("(Unknown)")
    , state_(&parser::advance_gen)
    , line_(0)
    , pos_(0)
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
            if (!skip_comment(e)) {
                return false;
            }
            continue;
        case '[':
            return advance_section(e);
        default:
            if (std::isspace(c)) continue;
            if (std::isalnum(c)) {
                put_back(c);
                return advance_param(e);
            }
            char buf[] = {'s', 'y', 'm', 'b', 'o', 'l', ' ', '\'', c, '\'', '\0'};
            unexpected_token(e, buf);
            return false;
        }
    }
}

bool parser::skip_comment(event & e)
{
    for (;;) {
        const char c = get_char();

        switch (c) {
        case '\r':
            check_lf();
        case '\n':
            handle_new_line();
            return in_.good();
        default:
            if (!in_) return false;
            /* Consuming symbols till the end of the string */
            continue;
        }
    }
}

bool parser::advance_section(event & e)
{
    e.value.clear();
    skip_ws();
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
                unexpected_token(e, "]");
            } else {
                e.type = EVENT_SECTION;
            }
            state_ = &parser::advance_gen;
            trim_right(e.value);
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
            trim_right(e.value);
            return true;
        default:
            e.value.push_back(c);
        }
    }
}

bool parser::advance_value(event & e)
{
    e.value.clear();
    skip_ws();
    for (;;) {
        const char c = get_char();

        if (in_.eof()) {
            // Empty values at the end of file are ok
            state_ = &parser::advance_eof;
            goto done;
        }

        switch (c) {
        case '\r':
            check_lf();
        case '\n':
            handle_new_line();
            state_ = &parser::advance_gen;
            goto done;
        case ';':
            if (!skip_comment(e)) {
                state_ = &parser::advance_eof;
            } else {
                state_ = &parser::advance_gen;
            }
            goto done;
        default:
            e.value.push_back(c);
        }
    }
    done:
    e.type = EVENT_VALUE;
    trim_right(e.value);
    return true;
}

bool parser::advance_eof(event & e)
{
    state_ = &parser::advance_eof;
    e.type = EVENT_END;
    e.value.clear();
    return false;
}

bool parser::handle_eof(event & e)
{
    const bool eof = in_.eof();
    if (eof) advance_eof(e);
    return eof;
}

bool parser::skip_ws()
{
    char c;
    while (in_ && std::isspace(c = get_char()));
    const bool ok = in_.good();
    if (ok) put_back(c);
    return ok;
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
    ss << filename_ << ":" << line_ << ":" << pos_ << ": Unexpected token: "
       << desc;
    e.type = EVENT_ERROR;
    e.value = ss.str();
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

}
}
