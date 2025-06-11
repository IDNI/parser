// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.txt

// JSON parser tutorial

#include <iostream>
#include <fstream>
#include <stack>
#include <optional>
#include <limits>

#include "parser.h"

using namespace std;
using namespace idni;

//state class for traversing a JSON collection element
struct traversal_state { 
    traversal_state(bool is_array) : 
        type(is_array), 
        size(0) {}

    bool is_array() {
        return this->type;
    }
    void add_element() {
        this->size++;
    }
    int elements() {
        return this->size;
    }
private:
    // true for array, false for object
    bool type = true;
    // number of elements so far in the collection
    int size = 0;
};

struct json_parser {
    json_parser() :
        // predefined character class function for digits, printables and spaces
        cc(predefined_char_classes({ "digit", "printable", "space" }, nts)),
        digit(nts("digit")), printable(nts("printable")), space(nts("space")), 
        // create nonterminals we will use
        start(nts("start")), element(nts("element")), whitespace(nts("whitespace")),
        integer(nts("integer")), digits(nts("digits")), 
        escaping(nts("escaping")), escaped(nts("escaped")),
        unescaped(nts("unescaped")), strchar(nts("strchar")),
        strchars(nts("strchars")), str(nts("string")),
        array(nts("array")), itemseq(nts("itemseq")),
        object(nts("object")), keyvalue(nts("keyvalue")), pairs(nts("pairs")),

        g(nts, rules(), start, cc), p(g) {}
    optional<string> parse(const char* data, size_t size) {
        auto res = p.parse(data, size);
        if (!res.found)	{
            cerr << res.parse_error << '\n';
            return {};
        }
        // return parsed JSON by calling get_value method which
        // traverses the forest and convert is back to string
        return get_value(res);
    }
private:
    nonterminals<> nts{};
    char_class_fns<> cc;
    // define nonterminals
    prods<> digit, printable, space,
        start, element, whitespace,
        integer, digits,
        escaping, escaped, unescaped,
        strchar, strchars, str,
        array, itemseq, object, keyvalue, pairs;
    grammar<> g;
    parser<> p;
    // rules for the grammar
    prods<> rules() {
        // define terminals outside predefined classes
        prods<> r, minus('-'), quote('"'), esc('\\'),
            opensquare('['), closedsquare(']'), comma(','),
            opencurly('{'), closedcurly('}'), colon(':'),
            nul{ lit() }; // nul(l) literal
        
        // whitespace is zero or more spaces
        r(whitespace, (space + whitespace) | nul);

        // digits is one or more individual digits
        r(digits,     digit | (digits + digit));
        // integer is either signed or unsigned
        r(integer,    digits | (minus + digits));

        // we are escaping quote (") or esc (\)
        r(escaping,   quote | esc);
        // unescaped are all printable but not those we are escaping
        r(unescaped,  printable & ~escaping);
        // escaped is escape and what we are escaping
        r(escaped,    esc + escaping);
        // string char is unescaped or escaped
        r(strchar,    unescaped | escaped);
        // strchars is a sequence of string characters or nothing
        r(strchars,   (strchar + strchars) | nul);
        // string is string characters in quotes but can be empty
        r(str,        quote + strchars + quote);

        // array items are zero or more comma separated elements
        r(itemseq,    element | 
                      (element + whitespace + comma + whitespace + itemseq) | 
                      nul);
        // array is list of elements in square brackets
        r(array,      opensquare + whitespace + itemseq + whitespace + closedsquare);

        // key-value pair are string and element separated by colon
        r(keyvalue,   str + whitespace + colon + whitespace + element);
        // object elements are zero or more comma separated key-value pairs
        r(pairs,      keyvalue | 
                      (keyvalue + whitespace + comma + whitespace + pairs) | 
                      nul);
        // object is list of key-value pairs in curly braces
        r(object,     opencurly + whitespace + pairs + whitespace + closedcurly);
        
        // JSON elemen it is either a number, string, array, or an object
        r(element,    integer | str | array | object);
        // start can be value
        r(start,      element);
        return r;
    }

    // traverses the parsed forest and reads a parsed value from it.
    string get_value(typename parser<>::result& res) {
        // indentation style
        const string indentation = string(2, ' ');
        // output string builder
        ostringstream buffer;
        // current indentation level
        string indentation_level;
        // states of nested collection element types
        stack<traversal_state> state;

        // define a callback called when the traversal enters a node n
        auto cb_enter = [&buffer, &indentation_level, &state, &indentation, &res, this](const parser<>::pnode& n) {
            // n is a pair of a literal and its span
            // we can compare the literal with literals predefined
            // as members of 'this' object: integer/str/array/object

            // current element output value
            string element;
            // potential new state for the stack
            // new state is pushed at the end becuase adding commas and colons
            // requires data for the current tip state
            optional<traversal_state> new_state;

            // if string or number, get node's terminals as a string
            if (n.first == integer || n.first == str) {
                element = res.get_terminals(n);
            }
            // if array, open the bracket and initiate new state
            else if (n.first == array) {
                element = "[";
                new_state = traversal_state(true);
            }
            // similarly for an object, open the bracket and initiate new state
            else if (n.first == object)  {
                element = "{";
                new_state = traversal_state(false);
            }

            // if we have an element to add to the output
            if (element.size() > 0) {
                if (!state.empty()) {
                    // get the state for the last open collection
                    auto &current_state = state.top();
                    // break line if we are adding the first element to a collection
                    // this ensures that emtpy collection have open and closed
                    // bracket in the same line
                    if (current_state.elements() == 0)
                        buffer << endl;

                    // if the state is object and we are expecting value part of
                    // a key-value pair
                    if (!current_state.is_array() && current_state.elements() % 2 == 1)
                        buffer << ": ";
                    // add coma after the first element of array or object
                    // key-value pair, and prepare indentation for the next
                    // item
                    else if (current_state.elements() > 0)
                        buffer << "," << endl << indentation_level;
                    // prepare indentation if this is a very first element
                    // of the collection
                    else
                        buffer << indentation_level;
                    
                    // advance the number of elements in the collection
                    // keys and values in objects count as separate elements
                    current_state.add_element();
                }

                // add the element to output
                buffer << element;
            }

            // push new state if any, and increase indentation
            if (new_state.has_value()) {
                state.push(new_state.value());
                indentation_level += indentation;
            }
        };

        // define a callback called when the traversal leaves a node n
        auto cb_exit = [&buffer, &indentation_level, &state, &indentation, this](
            const parser<>::pnode& n, const forest<parser<>::pnode>::nodes_set &)
        {
            // n is a pair of a literal and its span

            // when exiting array, reduce indentation and add closing bracket
            if (n.first == array) {
                // remove one indentation level
                indentation_level.resize(indentation_level.size() - indentation.size());
                // if the collection was non-empty close bracket in the new line
                // with reduced indentation
                if (state.top().elements() > 0)
                    buffer << endl << indentation_level;
                // add closing bracket
                buffer << "]";
                // go back to the state of an outer collection
                state.pop();
            }
            // similarly for an object, reduce indentation and add closing bracket
            else if (n.first == object) {
                indentation_level.resize(indentation_level.size() - indentation.size());
                if (state.top().elements() > 0)
                    buffer << endl << indentation_level;
                buffer << "}";
                state.pop();
            }
        };

        // run traversal with enter and exit callbacks
        res.get_forest()->traverse(cb_enter, cb_exit);
        // convert string stream to string
        return buffer.str();
    }
};

int main(int argc, char *argv[]) {
    cout << "Validator for JSON files." << endl;
    if (argc != 2) {
        cout << "Provide a file as an argument" << endl;
        return 0;
    }

    // open file
    ifstream reader(argv[1]);
    if (!reader.good()) {
        cerr << "File could not be opened." << endl;
    }

    // read entire file to string
    std::ostringstream stringstream;
    stringstream << reader.rdbuf();
    auto input = stringstream.str();
    reader.close();

    // instantiate JSON parser and parse the string
    json_parser p;
    auto res = p.parse(input.c_str(), input.length());

    if (!res) {
        cout << "invalid" << endl;
        return 0;
    } else {
        // recreate a JSON string from the parsed tree
        auto& v = res.value();
        cout << "Parsed value: " << v << endl;

        // check if recreated string is valid
        auto res2 = p.parse(v.c_str(), v.length());
        if (!res2) {
            cout << "traversal output is invalid" << endl;
        }
    }

    return 0;
}
