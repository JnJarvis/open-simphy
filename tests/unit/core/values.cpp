#include <catch2/catch_test_macros.hpp>
#include <opensim/core/values.hpp>
using namespace opensim::core;
TEST_CASE("stable diagnostic names and independent owned text") {
    CHECK(std::string(code_name(Code::invalid_argument)) == "invalid_argument");
    CHECK(std::string(code_name(Code::id_exhausted)) == "id_exhausted");
    CHECK(std::string(code_name(Code::duplicate_id)) == "duplicate_id");
    CHECK(std::string(code_name(Code::missing_reference)) == "missing_reference");
    CHECK(std::string(code_name(Code::unsupported_feature)) == "unsupported_feature");
    CHECK(std::string(code_name(Code::invalid_data)) == "invalid_data");
    CHECK(std::string(code_name(Code::internal_error)) == "internal_error");
    Diagnostic a{Code::invalid_data, Severity::error, "original", {}, "field"};
    auto b = Result<int>::failure(a);
    a.message = "changed";
    a.path = "other";
    CHECK(b.error()->message == "original");
    CHECK(b.error()->path == "field");
}
