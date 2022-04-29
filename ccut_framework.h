#ifndef CCUT_FRAMEWORK_H
#define CCUT_FRAMEWORK_H

#include <map>
#include <vector>
#include <initializer_list>
#include <string>
#include <iostream>
#include <sstream>
#include <exception>
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
static inline std::string ansi(const std::initializer_list<colors>& codes)
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
static inline std::ostream& operator<<(std::ostream& os, colors code)
{
    return os << ansi({code});
}

// Helpful operator for outputting one ansi escape code
static inline std::ostream& operator<<(std::ostream& os, std::initializer_list<colors> codes)
{
    return os << ansi(codes);
}

// All tests to be run in test_main()
static std::map<std::string, test_func_t> tests;

// Register a new test
class RegisterTest
{
public:
    inline RegisterTest(std::string name, test_func_t func)
    {
        tests.emplace(name, func);
    }
};

// Custom exception class
class ccut_exception
{
public:
    inline ccut_exception(std::string reason, int line)
        : reason(reason)
        , line(line)
    {}

    inline std::string what() const
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
static inline int test_main()
{
    //                    funcname     reason
    std::vector<std::pair<std::string, std::string>> failures;

    // Run all tests
    for (const auto& test : tests)
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
            failures.push_back({test.first, std::string("Unexpected exception: ") + e.what()});
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
        std::cout << "\n- - - Failures - - -\n";
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

static inline void assert_true(bool expr, std::string str, int line)
{
    if (!expr)
        throw ccut_exception("Expected TRUE, but was FALSE: \"" + str + '"', line);
}

static inline void assert_false(bool expr, std::string str, int line)
{
    if (expr)
        throw ccut_exception("Expected FALSE, but was TRUE: \"" + str + '"', line);
}

template <typename T1, typename T2>
static inline void assert_equal(const T1& lhs, const T2& rhs, std::string lhs_str, std::string rhs_str, int line)
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
static inline void assert_unequal(const T1& lhs, const T2& rhs, std::string lhs_str, std::string rhs_str, int line)
{
    if (!(lhs != rhs))
    {
        std::ostringstream os;
        os << "Expected UNEQUAL, but was NOT UNEQUAL: [" << lhs_str << "]"
           << " and [" << rhs_str << "]";
        throw ccut_exception(os.str(), line);
    }
}

static inline void assert_almost_equal(long double lhs, long double rhs, std::string lhs_str, std::string rhs_str, int line)
{
    static constexpr long double allowable_error = 0.0001;
    double real_error = std::abs(lhs - rhs);
    if (real_error > allowable_error)
    {
        std::ostringstream os;
        os << "Expected ALMOST EQUAL, but was NOT ALMOST EQUAL: [" << lhs_str << "]"
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
    catch (const std::exception& e)              \
    {                                            \
        varname = true;                          \
    }

#define CCUT_CONCAT_IMPL(x, y) x##y
#define CCUT_CONCAT(x, y) CCUT_CONCAT_IMPL(x, y)

#define CCUT_ASSERT_EXCEPTION_IMPL(func_call)                                      \
    CCUT_DETERMINE_THROW(func_call, CCUT_CONCAT(ccut_threw_, __LINE__))            \
    if (!CCUT_CONCAT(ccut_threw_, __LINE__))                                       \
    {                                                                              \
        std::ostringstream os;                                                     \
        os << "Expected EXCEPTION, but got NO EXCEPTION: \"" << #func_call << '"'; \
        throw ccut_framework::ccut_exception(os.str(), __LINE__);                  \
    }

#define CCUT_ASSERT_NO_EXCEPTION_IMPL(func_call)                                   \
    CCUT_DETERMINE_THROW(func_call, CCUT_CONCAT(ccut_threw_, __LINE__))            \
    if (CCUT_CONCAT(ccut_threw_, __LINE__))                                        \
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
#define ASSERT_ALMOST_EQUAL( lhs, rhs ) ccut_framework::assert_almost_equal(lhs, rhs, #lhs, #rhs, __LINE__)
#define ASSERT_EXCEPTION( func_call ) CCUT_ASSERT_EXCEPTION_IMPL(func_call)
#define ASSERT_NO_EXCEPTION( func_call ) CCUT_ASSERT_NO_EXCEPTION_IMPL(func_call)

// Declare a new test function
#define TEST(funcname)                                                                                      \
    static inline void funcname();                                                      /* declare test */  \
    static ccut_framework::RegisterTest register_ccut_##funcname(#funcname, &funcname); /* register test */ \
    void funcname()                                                                     /* implement test */

// Run main test script
#define TEST_MAIN() int main() { return ccut_framework::test_main(); }

} // namespace ccut_framework

#endif // ifndef CCUT_FRAMEWORK_H
