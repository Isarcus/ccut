#ifndef CCUT_FRAMEWORK_H
#define CCUT_FRAMEWORK_H

#include <exception>
#include <string>
#include <map>
#include <vector>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <cassert>

namespace ccut_framework
{

// Test function type
typedef void(*test_func_t)();

// Helpful reference for terminal formatting
enum class colors
{
    none = 0,
    bold = 1,
    red = 31,
    green = 32,
    yellow = 33,
};

// Output the specified escape code(s) to a stream
std::string ansi(const std::initializer_list<colors> &codes)
{
    // Doesn't make sense to call with no codes
    assert(codes.end() - codes.begin());

    // Output stream for code
    std::ostringstream os;

    // Begin escape sequence
    os  << "\033[";

    for (const colors* ptr = codes.begin(); ptr < codes.end(); ptr++)
    {
        os << static_cast<int>(*ptr);
        if (ptr + 1 < codes.end())
        {
            os << ';';
        }
    }

    // End message
    os << 'm';

    return os.str();
}

// Helpful operator for outputting one ansi escape code
std::ostream& operator<<(std::ostream& os, colors code)
{
    return os << ansi({code});
}

// Helpful operator for outputting one ansi escape code
std::ostream &operator<<(std::ostream &os, std::initializer_list<colors> codes)
{
    return os << ansi(codes);
}

static std::map<std::string, test_func_t> tests;

// Register a new test
class RegisterTest
{
public:
    RegisterTest(std::string name, test_func_t func)
    {
        tests.emplace(name, func);
    }
};

// Custom exception class
class ccut_exception
{
public:
    ccut_exception(std::string reason, int line)
        : reason(reason)
        , line(line)
    {}

    std::string what() const
    {
        std::ostringstream os;
        os << "Line " << colors::bold << line << colors::none << ": " << reason;
        return os.str();
    }

private:
    std::string reason;
    int line;
};

// Run all tests
static int test_main()
{
    //                    funcname     reason
    std::vector<std::pair<std::string, std::string>> failures;

    // Run all tests
    for (const auto &test : tests)
    {
        std::cout << "Running test \"" + test.first + "\" . . . ";

        try
        {
            test.second();
            std::cout << colors::green << "PASS\n" << colors::none;
        }
        catch (const ccut_exception& ce)
        {
            std::cout << colors::red << "FAIL\n" << colors::none;
            failures.push_back({test.first, ce.what()});
        }
        catch (const std::exception& e)
        {
            std::cout << colors::yellow << "EXCEPTION\n" << colors::none;
            failures.push_back({test.first, e.what()});
        }
        catch (...)
        {
            std::cout << ansi({colors::red, colors::bold}) << "UNRECOGNIZED EXCEPTION\n" << colors::none;
            failures.push_back({test.first, "Totally unknown error was thrown!"});
        }
    }

    // Print failure reasons, if any
    if (failures.size())
    {
        std::cout << "- - - Failures - - -\n";
        for (const auto& fail : failures)
        {
            //                  [function name]         why it failed
            std::cout << " -> [" << fail.first << "] " << fail.second << "\n";
        }
    }
    std::cout << "\n";

    // Print overall summary
    std::cout << "Total passed: [" << tests.size() - failures.size() << " / " << tests.size() << "]\n";

    return 0;
}

//                     //
// Assertion Functions //
//                     //

void assert_true(bool expr, std::string str, int line)
{
    if (!expr)
        throw ccut_exception("Expected TRUE, but was FALSE: \"" + str + '"', line);
}

void assert_false(bool expr, std::string str, int line)
{
    if (expr)
        throw ccut_exception("Expected FALSE, but was TRUE: \"" + str + '"', line);
}

template <typename T1, typename T2>
void assert_equal(T1 lhs, T2 rhs, std::string lhs_str, std::string rhs_str, int line)
{
    if (!(lhs == rhs))
    {
        std::ostringstream os;
        os << "Expected EQUAL, but was NOT EQUAL: [" << lhs_str << "]"
           << " and [" << rhs_str << "]";
        throw ccut_exception(os.str(), line);
    }
}

template <typename T1, typename T2>
void assert_unequal(T1 lhs, T2 rhs, std::string lhs_str, std::string rhs_str, int line)
{
    if (!(lhs != rhs))
    {
        std::ostringstream os;
        os << "Expected UNEQUAL, but was NOT UNEQUAL: [" << lhs_str << "]"
           << " and [" << rhs_str << "]";
        throw ccut_exception(os.str(), line);
    }
}

#define CCUT_DETERMINE_THROW(func_call, varname) \
    bool varname = false;                        \
    try                                          \
    {                                            \
        func_call;                               \
    }                                            \
    catch (const std::exception &e)              \
    {                                            \
        varname = true;                          \
    }

#define CCUT_ASSERT_EXCEPTION_IMPL(func_call)                                      \
    CCUT_DETERMINE_THROW(func_call, threw)                                         \
    if (!threw)                                                                    \
    {                                                                              \
        std::ostringstream os;                                                     \
        os << "Expected EXCEPTION, but got NO EXCEPTION: \"" << #func_call << '"'; \
        throw ccut_framework::ccut_exception(os.str(), __LINE__);                  \
    }

#define CCUT_ASSERT_NO_EXCEPTION_IMPL(func_call)                                   \
    CCUT_DETERMINE_THROW(func_call, threw)                                         \
    if (threw)                                                                     \
    {                                                                              \
        std::ostringstream os;                                                     \
        os << "Expected NO EXCEPTION, but got EXCEPTION: \"" << #func_call << '"'; \
        throw ccut_framework::ccut_exception(os.str(), __LINE__);                  \
    }

//                  //
// Assertion Macros //
//                  //

#define ASSERT_TRUE( statement ) ccut_framework::assert_true(statement, #statement, __LINE__)
#define ASSERT_FALSE( statement ) ccut_framework::assert_false(statement, #statement, __LINE__)
#define ASSERT_EQUAL( lhs, rhs ) ccut_framework::assert_equal(lhs, rhs, #lhs, #rhs, __LINE__)
#define ASSERT_UNEQUAL( lhs, rhs ) ccut_framework::assert_unequal(lhs, rhs, #lhs, #rhs, __LINE__)
#define ASSERT_EXCEPTION( func_call ) CCUT_ASSERT_EXCEPTION_IMPL(func_call)
#define ASSERT_NO_EXCEPTION( func_call ) CCUT_ASSERT_NO_EXCEPTION_IMPL(func_call)

// Declare a new test function
#define TEST(funcname)                                                                               \
    void funcname();                                                             /* declare test */  \
    ccut_framework::RegisterTest register_ccut_##funcname(#funcname, &funcname); /* register test */ \
    void funcname()                                                              /* implement test */

// Run main
#define TEST_MAIN() int main() { return ccut_framework::test_main(); }

} // namespace ccut_framework

#endif // ifndef CCUT_UNIT_TEST_FRAMEWORK_H