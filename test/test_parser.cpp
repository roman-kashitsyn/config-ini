#define BOOST_TEST_MODULE ini_parser

#include "config/ini/parser.hpp"
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <sstream>

using config::ini::parser;

BOOST_AUTO_TEST_CASE(simple_events)
{
    const parser::event expected[] = {
        { parser::EVENT_SECTION, "section" },
        { parser::EVENT_NAME, "param1" },
        { parser::EVENT_VALUE, "value1" },
        { parser::EVENT_NAME, "param2" },
        { parser::EVENT_VALUE, "value2" },
    };
    const std::size_t num_events = sizeof(expected) / sizeof (expected[0]);

    std::string content = "[section]\r\n"
        "param1=value1\r\n"
        "; some comment\r\n"
        "param2=value2\r\n";
    std::istringstream is(content);

    parser p(is);
    parser::event e;

    for (std::size_t i = 0; i < num_events; ++i) {
        BOOST_CHECK(p.advance(e));
        std::cout << "step " << i+1 << "\n"
                  << "parsed:  " << e << "\n"
                  << "expeced: " << expected[i] << std::endl;
        BOOST_CHECK(e == expected[i]);
    }

    BOOST_ASSERT(!p.advance(e));
}
