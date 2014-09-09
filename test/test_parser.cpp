#define BOOST_TEST_MODULE ini_parser

#include "config/ini/parser.hpp"
#include <boost/test/included/unit_test.hpp>
#include <sstream>

using config::ini::parser;

BOOST_AUTO_TEST_CASE(test_simple_event_sequence)
{
    const parser::event expected[] = {
        { parser::EVENT_SECTION, "section" },
        { parser::EVENT_NAME,    "param1" },
        { parser::EVENT_VALUE,   "value1" },
        { parser::EVENT_NAME,    "param2" },
        { parser::EVENT_VALUE,   "value2" },
        { parser::EVENT_SECTION, "section 2" },
        { parser::EVENT_NAME,    "param3" },
        { parser::EVENT_VALUE,   "value3"},
    };
    const std::size_t num_events = sizeof(expected) / sizeof (expected[0]);

    std::string content = "[section]\r\n"
        "param1=value1\r\n"
        "; some comment\r\n"
        "param2 = value2 \r\n"
        "\t\n"
        "[ section 2 ] ;comment\n"
        "param3 = value3 ; inline comment";
    std::istringstream is(content);

    parser p(is);
    parser::event e;

    for (std::size_t i = 0; i < num_events; ++i) {
        BOOST_CHECK(p.advance(e));
        BOOST_CHECK(e == expected[i]);
    }

    BOOST_ASSERT(!p.advance(e));
}

BOOST_AUTO_TEST_CASE(test_unexpected_end_of_section)
{
    std::istringstream is("[section");
    std::string filename("section.ini");
    parser p(filename, is);
    parser::event e;
    BOOST_CHECK(!p.advance(e));
    BOOST_CHECK(e.type == parser::EVENT_ERROR);
    BOOST_CHECK(e.value.find(filename) == 0);
    BOOST_CHECK(e.value.find("end of file") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_unexpected_starting_symbol)
{
    std::istringstream is("\n!section!\n");
    parser p(is);
    parser::event e;
    BOOST_CHECK(!p.advance(e));
    BOOST_CHECK(e.type == parser::EVENT_ERROR);
    BOOST_MESSAGE(e.value);
    BOOST_CHECK(e.value.find("symbol '!'") != std::string::npos);
}
