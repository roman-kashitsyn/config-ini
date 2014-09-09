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
#ifndef CONFIG_INI_PARSER_HPP
#define CONFIG_INI_PARSER_HPP

#include <iosfwd>
#include <string>

namespace config {
namespace ini {

/**
 * @brief Pull .ini file parser implementation.
 */
class parser
{
public:
    enum event_type
    {
        EVENT_SECTION,
        EVENT_NAME,
        EVENT_VALUE,
        EVENT_ERROR,
        EVENT_END
    };

    struct event
    {
        event_type type;
        std::string value;
    };

    /**
     * @brief Constructs parser of input stream \p in.
     */
    parser(std::istream &in);

    /**
     * @brief Constructs parser of input stream \p in assuming that
     * filename is \p filename.
     */
    parser(const std::string &filename, std::istream &in);

    /**
     * @brief Retrieves next parser event from the input stream.
     * @param e event to modify
     * @return true if a useful event was retrieved,
     *         false on error or end of stream
     */
    bool advance(event & e);

private:
    // noncopyable
    parser(const parser &);
    parser & operator=(const parser &);

    char get_char();
    void put_back(char);

    bool advance_gen(event &);
    bool advance_section(event &);
    bool advance_param(event &);
    bool advance_value(event &);
    bool advance_eof(event &);

    bool handle_eof(event &);
    bool skip_ws();
    bool skip_comment(event &);
    void unexpected_token(event &, const char *);
    void check_lf();
    void handle_new_line();

    typedef bool (parser::*state)(event &);

    std::istream & in_;
    std::string filename_;
    state state_;
    std::size_t line_;
    std::size_t column_;
};

bool operator==(const parser::event &,
                const parser::event &);

std::ostream & operator<<(std::ostream &,
                          const parser::event &);

}
}

#endif
