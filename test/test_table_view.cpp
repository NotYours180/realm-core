/*************************************************************************
 *
 * Copyright 2016 Realm Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************/

#include "testsettings.hpp"
#ifdef TEST_TABLE_VIEW

#include <limits>
#include <string>
#include <sstream>
#include <ostream>
#include <cwchar>

#include <realm/table_macros.hpp>

#include "util/misc.hpp"

#include "test.hpp"

using namespace realm;
using namespace test_util;


// Test independence and thread-safety
// -----------------------------------
//
// All tests must be thread safe and independent of each other. This
// is required because it allows for both shuffling of the execution
// order and for parallelized testing.
//
// In particular, avoid using std::rand() since it is not guaranteed
// to be thread safe. Instead use the API offered in
// `test/util/random.hpp`.
//
// All files created in tests must use the TEST_PATH macro (or one of
// its friends) to obtain a suitable file system path. See
// `test/util/test_path.hpp`.
//
//
// Debugging and the ONLY() macro
// ------------------------------
//
// A simple way of disabling all tests except one called `Foo`, is to
// replace TEST(Foo) with ONLY(Foo) and then recompile and rerun the
// test suite. Note that you can also use filtering by setting the
// environment varible `UNITTEST_FILTER`. See `README.md` for more on
// this.
//
// Another way to debug a particular test, is to copy that test into
// `experiments/testcase.cpp` and then run `sh build.sh
// check-testcase` (or one of its friends) from the command line.


namespace {

REALM_TABLE_1(TestTableInt, first, Int)

REALM_TABLE_2(TestTableInt2, first, Int, second, Int)

REALM_TABLE_2(TestTableDate, first, OldDateTime, second, Int)

REALM_TABLE_2(TestTableFloatDouble, first, Float, second, Double)


} // anonymous namespace


TEST(TableView_Json)
{
    Table table;
    table.add_column(type_Int, "first");

    size_t ndx = table.add_empty_row();
    table.set_int(0, ndx, 1);
    ndx = table.add_empty_row();
    table.set_int(0, ndx, 2);
    ndx = table.add_empty_row();
    table.set_int(0, ndx, 3);

    TableView v = table.where().find_all(1);
    std::stringstream ss;
    v.to_json(ss);
    const std::string json = ss.str();
    CHECK_EQUAL(true, json.length() > 0);
    CHECK_EQUAL("[{\"first\":2},{\"first\":3}]", json);
}


TEST(TableView_DateMaxMin)
{
    TestTableDate ttd;

    ttd.add(OldDateTime(2014, 7, 10), 1);
    ttd.add(OldDateTime(2013, 7, 10), 1);
    ttd.add(OldDateTime(2015, 8, 10), 1);
    ttd.add(OldDateTime(2015, 7, 10), 1);

    TestTableDate::View v = ttd.column().second.find_all(1);
    size_t ndx = not_found;

    CHECK_EQUAL(OldDateTime(2015, 8, 10), v.column().first.maximum(&ndx));
    CHECK_EQUAL(2, ndx);

    CHECK_EQUAL(OldDateTime(2013, 7, 10), v.column().first.minimum(&ndx));
    CHECK_EQUAL(1, ndx);
}

TEST(TableView_TimestampMaxMinCount)
{
    Table t;
    t.add_column(type_Timestamp, "ts", true);
    t.add_empty_row();
    t.set_timestamp(0, 0, Timestamp(300, 300));

    t.add_empty_row();
    t.set_timestamp(0, 1, Timestamp(100, 100));

    t.add_empty_row();
    t.set_timestamp(0, 2, Timestamp(200, 200));

    // Add row with null. For max(), any non-null is greater, and for min() any non-null is less
    t.add_empty_row();

    TableView tv = t.where().find_all();
    Timestamp ts;

    ts = tv.maximum_timestamp(0, nullptr);
    CHECK_EQUAL(ts, Timestamp(300, 300));
    ts = tv.minimum_timestamp(0, nullptr);
    CHECK_EQUAL(ts, Timestamp(100, 100));

    size_t index;
    ts = tv.maximum_timestamp(0, &index);
    CHECK_EQUAL(index, 0);
    ts = tv.minimum_timestamp(0, &index);
    CHECK_EQUAL(index, 1);

    size_t cnt;
    cnt = tv.count_timestamp(0, Timestamp(100, 100));
    CHECK_EQUAL(cnt, 1);

    cnt = tv.count_timestamp(0, Timestamp{});
    CHECK_EQUAL(cnt, 1);
}

TEST(TableView_TimestampGetSet)
{
    Table t;
    t.add_column(type_Timestamp, "ts", true);
    t.add_empty_row(3);
    t.set_timestamp(0, 0, Timestamp(000, 010));
    t.set_timestamp(0, 1, Timestamp(100, 110));
    t.set_timestamp(0, 2, Timestamp(200, 210));

    TableView tv = t.where().find_all();
    CHECK_EQUAL(tv.get_timestamp(0, 0), Timestamp(000, 010));
    CHECK_EQUAL(tv.get_timestamp(0, 1), Timestamp(100, 110));
    CHECK_EQUAL(tv.get_timestamp(0, 2), Timestamp(200, 210));

    tv.set_timestamp(0, 0, Timestamp(1000, 1010));
    tv.set_timestamp(0, 1, Timestamp(1100, 1110));
    tv.set_timestamp(0, 2, Timestamp(1200, 1210));
    CHECK_EQUAL(tv.get_timestamp(0, 0), Timestamp(1000, 1010));
    CHECK_EQUAL(tv.get_timestamp(0, 1), Timestamp(1100, 1110));
    CHECK_EQUAL(tv.get_timestamp(0, 2), Timestamp(1200, 1210));
}

TEST(TableView_GetSetInteger)
{
    TestTableInt table;

    table.add(1);
    table.add(2);
    table.add(3);
    table.add(1);
    table.add(2);

    TestTableInt::View v;                 // Test empty construction
    v = table.column().first.find_all(2); // Test assignment

    CHECK_EQUAL(2, v.size());

    // Test of Get
    CHECK_EQUAL(2, v[0].first);
    CHECK_EQUAL(2, v[1].first);

    // Test of Set
    v[0].first = 123;
    CHECK_EQUAL(123, v[0].first);
}


namespace {
REALM_TABLE_3(TableFloats, col_float, Float, col_double, Double, col_int, Int)
}

TEST(TableView_FloatsGetSet)
{
    TableFloats table;

    float f_val[] = {1.1f, 2.1f, 3.1f, -1.1f, 2.1f, 0.0f};
    double d_val[] = {1.2, 2.2, 3.2, -1.2, 2.3, 0.0};

    CHECK_EQUAL(true, table.is_empty());

    // Test add(?,?) with parameters
    for (size_t i = 0; i < 5; ++i)
        table.add(f_val[i], d_val[i], i);
    table.add();
    CHECK_EQUAL(6, table.size());
    for (size_t i = 0; i < 6; ++i) {
        CHECK_EQUAL(f_val[i], table.column().col_float[i]);
        CHECK_EQUAL(d_val[i], table.column().col_double[i]);
    }

    TableFloats::View v;                         // Test empty construction
    v = table.column().col_float.find_all(2.1f); // Test assignment
    CHECK_EQUAL(2, v.size());

    TableFloats::View v2(v);


    // Test of Get
    CHECK_EQUAL(2.1f, v[0].col_float);
    CHECK_EQUAL(2.1f, v[1].col_float);
    CHECK_EQUAL(2.2, v[0].col_double);
    CHECK_EQUAL(2.3, v[1].col_double);

    // Test of Set
    v[0].col_float = 123.321f;
    CHECK_EQUAL(123.321f, v[0].col_float);
    v[0].col_double = 123.3219;
    CHECK_EQUAL(123.3219, v[0].col_double);
}

TEST(TableView_FloatsFindAndAggregations)
{
    TableFloats table;
    float f_val[] = {1.2f, 2.1f, 3.1f, -1.1f, 2.1f, 0.0f};
    double d_val[] = {-1.2, 2.2, 3.2, -1.2, 2.3, 0.0};
    // v_some =        ^^^^              ^^^^
    double sum_f = 0.0;
    double sum_d = 0.0;
    for (size_t i = 0; i < 6; ++i) {
        table.add(f_val[i], d_val[i], 1);
        sum_d += d_val[i];
        sum_f += f_val[i];
    }

    // Test find_all()
    TableFloats::View v_all = table.column().col_int.find_all(1);
    CHECK_EQUAL(6, v_all.size());

    TableFloats::View v_some = table.column().col_double.find_all(-1.2);
    CHECK_EQUAL(2, v_some.size());
    CHECK_EQUAL(0, v_some.get_source_ndx(0));
    CHECK_EQUAL(3, v_some.get_source_ndx(1));

    // Test find_first
    CHECK_EQUAL(0, v_all.column().col_double.find_first(-1.2));
    CHECK_EQUAL(5, v_all.column().col_double.find_first(0.0));
    CHECK_EQUAL(2, v_all.column().col_double.find_first(3.2));

    CHECK_EQUAL(1, v_all.column().col_float.find_first(2.1f));
    CHECK_EQUAL(5, v_all.column().col_float.find_first(0.0f));
    CHECK_EQUAL(2, v_all.column().col_float.find_first(3.1f));

    // TODO: add for float as well

    double epsilon = std::numeric_limits<double>::epsilon();

    // Test sum
    CHECK_APPROXIMATELY_EQUAL(sum_d, v_all.column().col_double.sum(), 10 * epsilon);
    CHECK_APPROXIMATELY_EQUAL(sum_f, v_all.column().col_float.sum(), 10 * epsilon);
    CHECK_APPROXIMATELY_EQUAL(-1.2 + -1.2, v_some.column().col_double.sum(), 10 * epsilon);
    CHECK_APPROXIMATELY_EQUAL(double(1.2f) + double(-1.1f), v_some.column().col_float.sum(), 10 * epsilon);

    size_t ndx = not_found;

    // Test max
    CHECK_EQUAL(3.2, v_all.column().col_double.maximum(&ndx));
    CHECK_EQUAL(2, ndx);

    CHECK_EQUAL(-1.2, v_some.column().col_double.maximum(&ndx));
    CHECK_EQUAL(0, ndx);

    CHECK_EQUAL(3.1f, v_all.column().col_float.maximum(&ndx));
    CHECK_EQUAL(2, ndx);

    CHECK_EQUAL(1.2f, v_some.column().col_float.maximum(&ndx));
    CHECK_EQUAL(0, ndx);

    // Max without ret_index
    CHECK_EQUAL(3.2, v_all.column().col_double.maximum());
    CHECK_EQUAL(-1.2, v_some.column().col_double.maximum());
    CHECK_EQUAL(3.1f, v_all.column().col_float.maximum());
    CHECK_EQUAL(1.2f, v_some.column().col_float.maximum());

    // Test min
    CHECK_EQUAL(-1.2, v_all.column().col_double.minimum());
    CHECK_EQUAL(-1.2, v_some.column().col_double.minimum());
    CHECK_EQUAL(-1.1f, v_all.column().col_float.minimum());
    CHECK_EQUAL(-1.1f, v_some.column().col_float.minimum());

    // min with ret_ndx
    CHECK_EQUAL(-1.2, v_all.column().col_double.minimum(&ndx));
    CHECK_EQUAL(0, ndx);

    CHECK_EQUAL(-1.2, v_some.column().col_double.minimum(&ndx));
    CHECK_EQUAL(0, ndx);

    CHECK_EQUAL(-1.1f, v_all.column().col_float.minimum(&ndx));
    CHECK_EQUAL(3, ndx);

    CHECK_EQUAL(-1.1f, v_some.column().col_float.minimum(&ndx));
    CHECK_EQUAL(1, ndx);

    // Test avg
    CHECK_APPROXIMATELY_EQUAL(sum_d / 6.0, v_all.column().col_double.average(), 10 * epsilon);
    CHECK_APPROXIMATELY_EQUAL((-1.2 + -1.2) / 2.0, v_some.column().col_double.average(), 10 * epsilon);
    CHECK_APPROXIMATELY_EQUAL(sum_f / 6.0, v_all.column().col_float.average(), 10 * epsilon);
    CHECK_APPROXIMATELY_EQUAL((double(1.2f) + double(-1.1f)) / 2, v_some.column().col_float.average(), 10 * epsilon);

    CHECK_EQUAL(1, v_some.column().col_float.count(1.2f));
    CHECK_EQUAL(2, v_some.column().col_double.count(-1.2));
    CHECK_EQUAL(2, v_some.column().col_int.count(1));

    CHECK_EQUAL(2, v_all.column().col_float.count(2.1f));
    CHECK_EQUAL(2, v_all.column().col_double.count(-1.2));
    CHECK_EQUAL(6, v_all.column().col_int.count(1));
}

TEST(TableView_Sum)
{
    TestTableInt table;

    table.add(2);
    table.add(2);
    table.add(2);
    table.add(2);
    table.add(2);

    TestTableInt::View v = table.column().first.find_all(2);
    CHECK_EQUAL(5, v.size());

    int64_t sum = v.column().first.sum();
    CHECK_EQUAL(10, sum);
}

TEST(TableView_Average)
{
    TestTableInt table;

    table.add(2);
    table.add(2);
    table.add(2);
    table.add(2);
    table.add(2);

    TestTableInt::View v = table.column().first.find_all(2);
    CHECK_EQUAL(5, v.size());

    double sum = v.column().first.average();
    CHECK_APPROXIMATELY_EQUAL(2., sum, 0.00001);
}

TEST(TableView_SumNegative)
{
    TestTableInt table;

    table.add(0);
    table.add(0);
    table.add(0);

    TestTableInt::View v = table.column().first.find_all(0);
    v[0].first = 11;
    v[2].first = -20;

    int64_t sum = v.column().first.sum();
    CHECK_EQUAL(-9, sum);
}

TEST(TableView_IsAttached)
{
    TestTableInt table;

    table.add(0);
    table.add(0);
    table.add(0);

    TestTableInt::View v = table.column().first.find_all(0);
    TestTableInt::View v2 = table.column().first.find_all(0);
    v[0].first = 11;
    CHECK_EQUAL(true, v.is_attached());
    CHECK_EQUAL(true, v2.is_attached());
    v.remove_last();
    CHECK_EQUAL(true, v.is_attached());
    CHECK_EQUAL(true, v2.is_attached());

    table.remove_last();
    CHECK_EQUAL(true, v.is_attached());
    CHECK_EQUAL(true, v2.is_attached());
}

TEST(TableView_Max)
{
    TestTableInt table;

    table.add(0);
    table.add(0);
    table.add(0);

    TestTableInt::View v = table.column().first.find_all(0);
    v[0].first = -1;
    v[1].first = 2;
    v[2].first = 1;

    int64_t max = v.column().first.maximum();
    CHECK_EQUAL(2, max);
}

TEST(TableView_Max2)
{
    TestTableInt table;

    table.add(0);
    table.add(0);
    table.add(0);

    TestTableInt::View v = table.column().first.find_all(0);
    v[0].first = -1;
    v[1].first = -2;
    v[2].first = -3;

    int64_t max = v.column().first.maximum();
    CHECK_EQUAL(-1, max);
}


TEST(TableView_Min)
{
    TestTableInt table;

    table.add(0);
    table.add(0);
    table.add(0);

    TestTableInt::View v = table.column().first.find_all(0);
    v[0].first = -1;
    v[1].first = 2;
    v[2].first = 1;

    int64_t min = v.column().first.minimum();
    CHECK_EQUAL(-1, min);

    size_t ndx = not_found;
    min = v.column().first.minimum(&ndx);
    CHECK_EQUAL(-1, min);
    CHECK_EQUAL(0, ndx);
}

TEST(TableView_Min2)
{
    TestTableInt table;

    table.add(0);
    table.add(0);
    table.add(0);

    TestTableInt::View v = table.column().first.find_all(0);
    v[0].first = -1;
    v[1].first = -2;
    v[2].first = -3;

    int64_t min = v.column().first.minimum();
    CHECK_EQUAL(-3, min);

    size_t ndx = not_found;
    min = v.column().first.minimum(&ndx);
    CHECK_EQUAL(-3, min);
    CHECK_EQUAL(2, ndx);
}


TEST(TableView_Find)
{
    TestTableInt table;

    table.add(0);
    table.add(0);
    table.add(0);

    TestTableInt::View v = table.column().first.find_all(0);
    v[0].first = 5;
    v[1].first = 4;
    v[2].first = 4;

    size_t r = v.column().first.find_first(4);
    CHECK_EQUAL(1, r);
}


TEST(TableView_Follows_Changes)
{
    Table table;
    table.add_column(type_Int, "first");
    table.add_empty_row();
    table.set_int(0, 0, 1);
    Query q = table.where().equal(0, 1);
    TableView v = q.find_all();
    CHECK_EQUAL(1, v.size());
    CHECK_EQUAL(1, v.get_int(0, 0));

    // low level sanity check that we can copy a query and run the copy:
    Query q2 = q;
    TableView v2 = q2.find_all();

    // now the fun begins
    CHECK_EQUAL(1, v.size());
    table.add_empty_row();
    CHECK_EQUAL(1, v.size());
    table.set_int(0, 1, 1);
    v.sync_if_needed();
    CHECK_EQUAL(2, v.size());
    CHECK_EQUAL(1, v.get_int(0, 0));
    CHECK_EQUAL(1, v.get_int(0, 1));
    table.set_int(0, 0, 7);
    v.sync_if_needed();
    CHECK_EQUAL(1, v.size());
    CHECK_EQUAL(1, v.get_int(0, 0));
    table.set_int(0, 1, 7);
    v.sync_if_needed();
    CHECK_EQUAL(0, v.size());
    table.set_int(0, 1, 1);
    v.sync_if_needed();
    CHECK_EQUAL(1, v.size());
    CHECK_EQUAL(1, v.get_int(0, 0));
}


TEST(TableView_Distinct_Follows_Changes)
{
    Table table;
    table.add_column(type_Int, "first");
    table.add_column(type_String, "second");
    table.add_search_index(0);

    table.add_empty_row(5);
    for (int i = 0; i < 5; ++i) {
        table.set_int(0, i, i);
        table.set_string(1, i, "Foo");
    }

    TableView distinct_ints = table.get_distinct_view(0);
    CHECK_EQUAL(5, distinct_ints.size());
    CHECK(distinct_ints.is_in_sync());

    // Check that adding a value that doesn't actually impact the
    // view still invalidates the view (which is inspected for now).
    table.add_empty_row();
    table.set_int(0, 5, 4);
    table.set_string(1, 5, "Foo");
    CHECK(!distinct_ints.is_in_sync());
    distinct_ints.sync_if_needed();
    CHECK(distinct_ints.is_in_sync());
    CHECK_EQUAL(5, distinct_ints.size());

    // Check that adding a value that impacts the view invalidates the view.
    distinct_ints.sync_if_needed();
    table.add_empty_row();
    table.set_int(0, 6, 10);
    table.set_string(1, 6, "Foo");
    CHECK(!distinct_ints.is_in_sync());
    distinct_ints.sync_if_needed();
    CHECK(distinct_ints.is_in_sync());
    CHECK_EQUAL(6, distinct_ints.size());
}


TEST(TableView_SyncAfterCopy)
{
    Table table;
    table.add_column(type_Int, "first");
    table.add_empty_row();
    table.set_int(0, 0, 1);

    // do initial query
    Query q = table.where().equal(0, 1);
    TableView v = q.find_all();
    CHECK_EQUAL(1, v.size());
    CHECK_EQUAL(1, v.get_int(0, 0));

    // move the tableview
    TableView v2 = v;
    CHECK_EQUAL(1, v2.size());

    // make a change
    size_t ndx2 = table.add_empty_row();
    table.set_int(0, ndx2, 1);

    // verify that the copied view sees the change
    v2.sync_if_needed();
    CHECK_EQUAL(2, v2.size());
}

TEST(TableView_FindAll)
{
    TestTableInt table;

    table.add(0);
    table.add(0);
    table.add(0);

    TestTableInt::View v = table.column().first.find_all(0);
    CHECK_EQUAL(3, v.size());
    v[0].first = 5;
    v[1].first = 4; // match
    v[2].first = 4; // match

    // todo, add creation to wrapper function in table.h
    TestTableInt::View v2 = v.column().first.find_all(4);
    CHECK_EQUAL(2, v2.size());
    CHECK_EQUAL(1, v2.get_source_ndx(0));
    CHECK_EQUAL(2, v2.get_source_ndx(1));
}

namespace {

REALM_TABLE_1(TestTableString, first, String)

} // anonymous namespace

TEST(TableView_FindAllString)
{
    TestTableString table;

    table.add("a");
    table.add("a");
    table.add("a");

    TestTableString::View v = table.column().first.find_all("a");
    v[0].first = "foo";
    v[1].first = "bar"; // match
    v[2].first = "bar"; // match

    // todo, add creation to wrapper function in table.h
    TestTableString::View v2 = v.column().first.find_all("bar");
    CHECK_EQUAL(1, v2.get_source_ndx(0));
    CHECK_EQUAL(2, v2.get_source_ndx(1));
}


NONCONCURRENT_TEST(TableView_StringSort)
{
    // WARNING: Do not use the C++11 method (set_string_compare_method(1)) on Windows 8.1 because it has a bug that
    // takes length in count when sorting ("b" comes before "aaaa"). Bug is not present in Windows 7.

    // Test of handling of unicode takes place in test_utf8.cpp
    TestTableString table;

    table.add("alpha");
    table.add("zebra");
    table.add("ALPHA");
    table.add("ZEBRA");

    // Core-only is default comparer
    TestTableString::View v = table.where().find_all();
    v.column().first.sort();
    CHECK_EQUAL("alpha", v[0].first);
    CHECK_EQUAL("ALPHA", v[1].first);
    CHECK_EQUAL("zebra", v[2].first);
    CHECK_EQUAL("ZEBRA", v[3].first);

    // Should be exactly the same as above because 0 was default already
    set_string_compare_method(STRING_COMPARE_CORE, nullptr);
    v.column().first.sort();
    CHECK_EQUAL("alpha", v[0].first);
    CHECK_EQUAL("ALPHA", v[1].first);
    CHECK_EQUAL("zebra", v[2].first);
    CHECK_EQUAL("ZEBRA", v[3].first);

    // Test descending mode
    v.column().first.sort(false);
    CHECK_EQUAL("alpha", v[3].first);
    CHECK_EQUAL("ALPHA", v[2].first);
    CHECK_EQUAL("zebra", v[1].first);
    CHECK_EQUAL("ZEBRA", v[0].first);

    // primitive C locale comparer. But that's OK since all we want to test is
    // if the callback is invoked
    bool got_called = false;
    auto comparer = [&](const char* s1, const char* s2) {
        got_called = true;
        return *s1 < *s2;
    };

    // Test if callback comparer works. Our callback is a primitive dummy-comparer
    set_string_compare_method(STRING_COMPARE_CALLBACK, comparer);
    v.column().first.sort();
    CHECK_EQUAL("ALPHA", v[0].first);
    CHECK_EQUAL("ZEBRA", v[1].first);
    CHECK_EQUAL("alpha", v[2].first);
    CHECK_EQUAL("zebra", v[3].first);
    CHECK_EQUAL(true, got_called);

#ifdef _MSC_VER
    // Try C++11 method which uses current locale of the operating system to give precise sorting. This C++11 feature
    // is currently (mid 2014) only supported by Visual Studio
    got_called = false;
    bool available = set_string_compare_method(STRING_COMPARE_CPP11, nullptr);
    if (available) {
        v.column().first.sort();
        CHECK_EQUAL("alpha", v[0].first);
        CHECK_EQUAL("ALPHA", v[1].first);
        CHECK_EQUAL("zebra", v[2].first);
        CHECK_EQUAL("ZEBRA", v[3].first);
        CHECK_EQUAL(false, got_called);
    }
#endif

    // Set back to default for use by other unit tests
    set_string_compare_method(STRING_COMPARE_CORE, nullptr);
}


TEST(TableView_FloatDoubleSort)
{
    TestTableFloatDouble t;

    t.add(1.0f, 10.0);
    t.add(3.0f, 30.0);
    t.add(2.0f, 20.0);
    t.add(0.0f, 5.0);

    TestTableFloatDouble::View tv = t.where().find_all();
    tv.column().first.sort();

    CHECK_EQUAL(0.0f, tv[0].first);
    CHECK_EQUAL(1.0f, tv[1].first);
    CHECK_EQUAL(2.0f, tv[2].first);
    CHECK_EQUAL(3.0f, tv[3].first);

    tv.column().second.sort();
    CHECK_EQUAL(5.0f, tv[0].second);
    CHECK_EQUAL(10.0f, tv[1].second);
    CHECK_EQUAL(20.0f, tv[2].second);
    CHECK_EQUAL(30.0f, tv[3].second);
}

TEST(TableView_DoubleSortPrecision)
{
    // Detect if sorting algorithm accidentially casts doubles to float somewhere so that precision gets lost
    TestTableFloatDouble t;

    double d1 = 100000000000.0;
    double d2 = 100000000001.0;

    // When casted to float, they are equal
    float f1 = static_cast<float>(d1);
    float f2 = static_cast<float>(d2);

    // If this check fails, it's a bug in this unit test, not in Realm
    CHECK_EQUAL(f1, f2);

    // First verify that our unit is guaranteed to find such a bug; that is, test if such a cast is guaranteed to give
    // bad sorting order. This is not granted, because an unstable sorting algorithm could *by chance* give the
    // correct sorting order. Fortunatly we use std::stable_sort which must maintain order on draws.
    t.add(f2, d2);
    t.add(f1, d1);

    TestTableFloatDouble::View tv = t.where().find_all();
    tv.column().first.sort();

    // Sort should be stable
    CHECK_EQUAL(f2, tv[0].first);
    CHECK_EQUAL(f1, tv[1].first);

    // If sort is stable, and compare makes a draw because the doubles are accidentially casted to float in Realm,
    // then
    // original order would be maintained. Check that it's not maintained:
    tv.column().second.sort();
    CHECK_EQUAL(d1, tv[0].second);
    CHECK_EQUAL(d2, tv[1].second);
}

TEST(TableView_SortNullString)
{
    Table t;
    t.add_column(type_String, "s", true);
    t.add_empty_row(4);
    t.set_string(0, 0, StringData("")); // empty string
    t.set_string(0, 1, realm::null());  // realm::null()
    t.set_string(0, 2, StringData("")); // empty string
    t.set_string(0, 3, realm::null());  // realm::null()

    TableView tv;

    tv = t.where().find_all();
    tv.sort(0);
    CHECK(tv.get_string(0, 0).is_null());
    CHECK(tv.get_string(0, 1).is_null());
    CHECK(!tv.get_string(0, 2).is_null());
    CHECK(!tv.get_string(0, 3).is_null());

    t.set_string(0, 0, StringData("medium medium medium medium"));

    tv = t.where().find_all();
    tv.sort(0);
    CHECK(tv.get_string(0, 0).is_null());
    CHECK(tv.get_string(0, 1).is_null());
    CHECK(!tv.get_string(0, 2).is_null());
    CHECK(!tv.get_string(0, 3).is_null());

    t.set_string(0, 0, StringData("long long long long long long long long long long long long long long"));

    tv = t.where().find_all();
    tv.sort(0);
    CHECK(tv.get_string(0, 0).is_null());
    CHECK(tv.get_string(0, 1).is_null());
    CHECK(!tv.get_string(0, 2).is_null());
    CHECK(!tv.get_string(0, 3).is_null());
}

TEST(TableView_Delete)
{
    TestTableInt table;

    table.add(1);
    table.add(2);
    table.add(1);
    table.add(3);
    table.add(1);

    TestTableInt::View v = table.column().first.find_all(1);
    CHECK_EQUAL(3, v.size());

    v.remove(1);
    CHECK_EQUAL(2, v.size());
    CHECK_EQUAL(0, v.get_source_ndx(0));
    CHECK_EQUAL(3, v.get_source_ndx(1));

    CHECK_EQUAL(4, table.size());
    CHECK_EQUAL(1, table[0].first);
    CHECK_EQUAL(2, table[1].first);
    CHECK_EQUAL(3, table[2].first);
    CHECK_EQUAL(1, table[3].first);

    v.remove(0);
    CHECK_EQUAL(1, v.size());
    CHECK_EQUAL(2, v.get_source_ndx(0));

    CHECK_EQUAL(3, table.size());
    CHECK_EQUAL(2, table[0].first);
    CHECK_EQUAL(3, table[1].first);
    CHECK_EQUAL(1, table[2].first);

    v.remove(0);
    CHECK_EQUAL(0, v.size());

    CHECK_EQUAL(2, table.size());
    CHECK_EQUAL(2, table[0].first);
    CHECK_EQUAL(3, table[1].first);
}

TEST(TableView_Clear)
{
    TestTableInt table;

    table.add(1);
    table.add(2);
    table.add(1);
    table.add(3);
    table.add(1);

    TestTableInt::View v = table.column().first.find_all(1);
    CHECK_EQUAL(3, v.size());

    v.clear();
    CHECK_EQUAL(0, v.size());

    CHECK_EQUAL(2, table.size());
    CHECK_EQUAL(2, table[0].first);
    CHECK_EQUAL(3, table[1].first);
}


// Verify that TableView::clear() can handle a detached ref,
// so that it can be used in an imperative setting
TEST(TableView_Imperative_Clear)
{
    Table t;
    t.add_column(type_Int, "i1");
    t.add_empty_row(3);
    t.set_int(0, 0, 7);
    t.set_int(0, 1, 13);
    t.set_int(0, 2, 29);

    TableView v = t.where().less(0, 20).find_all();
    CHECK_EQUAL(2, v.size());
    // remove the underlying entry in the table, introducing a detached ref
    t.move_last_over(v.get_source_ndx(0));
    // the detached ref still counts as an entry when calling size()
    CHECK_EQUAL(2, v.size());
    // but is does not count as attached anymore:
    CHECK_EQUAL(1, v.num_attached_rows());
    v.clear();
    CHECK_EQUAL(0, v.size());
    CHECK_EQUAL(1, t.size());
}

// exposes a bug in stacked tableview:
// view V1 selects a subset of rows from Table T1
// View V2 selects rows from  view V1
// Then, some rows in V2 can be found, that are not in V1
TEST(TableView_Stacked)
{
    Table t;
    t.add_column(type_Int, "i1");
    t.add_column(type_Int, "i2");
    t.add_column(type_String, "S1");
    t.add_empty_row(2);
    t.set_int(0, 0, 1);      // 1
    t.set_int(1, 0, 2);      // 2
    t.set_string(2, 0, "A"); // "A"
    t.set_int(0, 1, 2);      // 2
    t.set_int(1, 1, 2);      // 2
    t.set_string(2, 1, "B"); // "B"

    TableView tv = t.find_all_int(0, 2);
    TableView tv2 = tv.find_all_int(1, 2);
    CHECK_EQUAL(1, tv2.size());             // evaluates tv2.size to 1 which is expected
    CHECK_EQUAL("B", tv2.get_string(2, 0)); // evalates get_string(2,0) to "A" which is not expected
}


TEST(TableView_ClearNone)
{
    TestTableInt table;

    TestTableInt::View v = table.column().first.find_all(1);
    CHECK_EQUAL(0, v.size());

    v.clear();
}


TEST(TableView_FindAllStacked)
{
    TestTableInt2 table;

    table.add(0, 1);
    table.add(0, 2);
    table.add(0, 3);
    table.add(1, 1);
    table.add(1, 2);
    table.add(1, 3);

    TestTableInt2::View v = table.column().first.find_all(0);
    CHECK_EQUAL(3, v.size());

    TestTableInt2::View v2 = v.column().second.find_all(2);
    CHECK_EQUAL(1, v2.size());
    CHECK_EQUAL(0, v2[0].first);
    CHECK_EQUAL(2, v2[0].second);
    CHECK_EQUAL(1, v2.get_source_ndx(0));
}


TEST(TableView_LowLevelSubtables)
{
    Table table;
    std::vector<size_t> column_path;
    table.add_column(type_Bool, "enable");
    table.add_column(type_Table, "subtab");
    table.add_column(type_Mixed, "mixed");
    column_path.push_back(1);
    table.add_subcolumn(column_path, type_Bool, "enable");
    table.add_subcolumn(column_path, type_Table, "subtab");
    table.add_subcolumn(column_path, type_Mixed, "mixed");
    column_path.push_back(1);
    table.add_subcolumn(column_path, type_Bool, "enable");
    table.add_subcolumn(column_path, type_Table, "subtab");
    table.add_subcolumn(column_path, type_Mixed, "mixed");

    table.add_empty_row(2 * 2);
    table.set_bool(0, 1, true);
    table.set_bool(0, 3, true);
    TableView view = table.where().equal(0, true).find_all();
    CHECK_EQUAL(2, view.size());
    for (int i_1 = 0; i_1 != 2; ++i_1) {
        TableRef subtab = view.get_subtable(1, i_1);
        subtab->add_empty_row(2 * (2 + i_1));
        for (int i_2 = 0; i_2 != 2 * (2 + i_1); ++i_2)
            subtab->set_bool(0, i_2, i_2 % 2 == 0);
        TableView subview = subtab->where().equal(0, true).find_all();
        CHECK_EQUAL(2 + i_1, subview.size());
        {
            TableRef subsubtab = subview.get_subtable(1, 0 + i_1);
            subsubtab->add_empty_row(2 * (3 + i_1));
            for (int i_3 = 0; i_3 != 2 * (3 + i_1); ++i_3)
                subsubtab->set_bool(0, i_3, i_3 % 2 == 1);
            TableView subsubview = subsubtab->where().equal(0, true).find_all();
            CHECK_EQUAL(3 + i_1, subsubview.size());

            for (int i_3 = 0; i_3 != 3 + i_1; ++i_3) {
                CHECK_EQUAL(true, bool(subsubview.get_subtable(1, i_3)));
                CHECK_EQUAL(false, bool(subsubview.get_subtable(2, i_3))); // Mixed
                CHECK_EQUAL(0, subsubview.get_subtable_size(1, i_3));
                CHECK_EQUAL(0, subsubview.get_subtable_size(2, i_3)); // Mixed
            }

            subview.clear_subtable(2, 1 + i_1); // Mixed
            TableRef subsubtab_mix = subview.get_subtable(2, 1 + i_1);
            subsubtab_mix->add_column(type_Bool, "enable");
            subsubtab_mix->add_column(type_Table, "subtab");
            subsubtab_mix->add_column(type_Mixed, "mixed");
            subsubtab_mix->add_empty_row(2 * (1 + i_1));
            for (int i_3 = 0; i_3 != 2 * (1 + i_1); ++i_3)
                subsubtab_mix->set_bool(0, i_3, i_3 % 2 == 0);
            TableView subsubview_mix = subsubtab_mix->where().equal(0, true).find_all();
            CHECK_EQUAL(1 + i_1, subsubview_mix.size());

            for (int i_3 = 0; i_3 != 1 + i_1; ++i_3) {
                CHECK_EQUAL(true, bool(subsubview_mix.get_subtable(1, i_3)));
                CHECK_EQUAL(false, bool(subsubview_mix.get_subtable(2, i_3))); // Mixed
                CHECK_EQUAL(0, subsubview_mix.get_subtable_size(1, i_3));
                CHECK_EQUAL(0, subsubview_mix.get_subtable_size(2, i_3)); // Mixed
            }
        }
        for (int i_2 = 0; i_2 != 2 + i_1; ++i_2) {
            CHECK_EQUAL(true, bool(subview.get_subtable(1, i_2)));
            CHECK_EQUAL(i_2 == 1 + i_1, bool(subview.get_subtable(2, i_2))); // Mixed
            CHECK_EQUAL(i_2 == 0 + i_1 ? 2 * (3 + i_1) : 0, subview.get_subtable_size(1, i_2));
            CHECK_EQUAL(i_2 == 1 + i_1 ? 2 * (1 + i_1) : 0, subview.get_subtable_size(2, i_2)); // Mixed
        }

        view.clear_subtable(2, i_1); // Mixed
        TableRef subtab_mix = view.get_subtable(2, i_1);
        std::vector<size_t> subcol_path;
        subtab_mix->add_column(type_Bool, "enable");
        subtab_mix->add_column(type_Table, "subtab");
        subtab_mix->add_column(type_Mixed, "mixed");
        subcol_path.push_back(1);
        subtab_mix->add_subcolumn(subcol_path, type_Bool, "enable");
        subtab_mix->add_subcolumn(subcol_path, type_Table, "subtab");
        subtab_mix->add_subcolumn(subcol_path, type_Mixed, "mixed");
        subtab_mix->add_empty_row(2 * (3 + i_1));
        for (int i_2 = 0; i_2 != 2 * (3 + i_1); ++i_2)
            subtab_mix->set_bool(0, i_2, i_2 % 2 == 1);
        TableView subview_mix = subtab_mix->where().equal(0, true).find_all();
        CHECK_EQUAL(3 + i_1, subview_mix.size());
        {
            TableRef subsubtab = subview_mix.get_subtable(1, 1 + i_1);
            subsubtab->add_empty_row(2 * (7 + i_1));
            for (int i_3 = 0; i_3 != 2 * (7 + i_1); ++i_3)
                subsubtab->set_bool(0, i_3, i_3 % 2 == 1);
            TableView subsubview = subsubtab->where().equal(0, true).find_all();
            CHECK_EQUAL(7 + i_1, subsubview.size());

            for (int i_3 = 0; i_3 != 7 + i_1; ++i_3) {
                CHECK_EQUAL(true, bool(subsubview.get_subtable(1, i_3)));
                CHECK_EQUAL(false, bool(subsubview.get_subtable(2, i_3))); // Mixed
                CHECK_EQUAL(0, subsubview.get_subtable_size(1, i_3));
                CHECK_EQUAL(0, subsubview.get_subtable_size(2, i_3)); // Mixed
            }

            subview_mix.clear_subtable(2, 2 + i_1); // Mixed
            TableRef subsubtab_mix = subview_mix.get_subtable(2, 2 + i_1);
            subsubtab_mix->add_column(type_Bool, "enable");
            subsubtab_mix->add_column(type_Table, "subtab");
            subsubtab_mix->add_column(type_Mixed, "mixed");
            subsubtab_mix->add_empty_row(2 * (5 + i_1));
            for (int i_3 = 0; i_3 != 2 * (5 + i_1); ++i_3)
                subsubtab_mix->set_bool(0, i_3, i_3 % 2 == 0);
            TableView subsubview_mix = subsubtab_mix->where().equal(0, true).find_all();
            CHECK_EQUAL(5 + i_1, subsubview_mix.size());

            for (int i_3 = 0; i_3 != 5 + i_1; ++i_3) {
                CHECK_EQUAL(true, bool(subsubview_mix.get_subtable(1, i_3)));
                CHECK_EQUAL(false, bool(subsubview_mix.get_subtable(2, i_3))); // Mixed
                CHECK_EQUAL(0, subsubview_mix.get_subtable_size(1, i_3));
                CHECK_EQUAL(0, subsubview_mix.get_subtable_size(2, i_3)); // Mixed
            }
        }
        for (int i_2 = 0; i_2 != 2 + i_1; ++i_2) {
            CHECK_EQUAL(true, bool(subview_mix.get_subtable(1, i_2)));
            CHECK_EQUAL(i_2 == 2 + i_1, bool(subview_mix.get_subtable(2, i_2))); // Mixed
            CHECK_EQUAL(i_2 == 1 + i_1 ? 2 * (7 + i_1) : 0, subview_mix.get_subtable_size(1, i_2));
            CHECK_EQUAL(i_2 == 2 + i_1 ? 2 * (5 + i_1) : 0, subview_mix.get_subtable_size(2, i_2)); // Mixed
        }

        CHECK_EQUAL(true, bool(view.get_subtable(1, i_1)));
        CHECK_EQUAL(true, bool(view.get_subtable(2, i_1))); // Mixed
        CHECK_EQUAL(2 * (2 + i_1), view.get_subtable_size(1, i_1));
        CHECK_EQUAL(2 * (3 + i_1), view.get_subtable_size(2, i_1)); // Mixed
    }


    ConstTableView const_view = table.where().equal(0, true).find_all();
    CHECK_EQUAL(2, const_view.size());
    for (int i_1 = 0; i_1 != 2; ++i_1) {
        ConstTableRef subtab = const_view.get_subtable(1, i_1);
        ConstTableView const_subview = subtab->where().equal(0, true).find_all();
        CHECK_EQUAL(2 + i_1, const_subview.size());
        {
            ConstTableRef subsubtab = const_subview.get_subtable(1, 0 + i_1);
            ConstTableView const_subsubview = subsubtab->where().equal(0, true).find_all();
            CHECK_EQUAL(3 + i_1, const_subsubview.size());
            for (int i_3 = 0; i_3 != 3 + i_1; ++i_3) {
                CHECK_EQUAL(true, bool(const_subsubview.get_subtable(1, i_3)));
                CHECK_EQUAL(false, bool(const_subsubview.get_subtable(2, i_3))); // Mixed
                CHECK_EQUAL(0, const_subsubview.get_subtable_size(1, i_3));
                CHECK_EQUAL(0, const_subsubview.get_subtable_size(2, i_3)); // Mixed
            }

            ConstTableRef subsubtab_mix = const_subview.get_subtable(2, 1 + i_1);
            ConstTableView const_subsubview_mix = subsubtab_mix->where().equal(0, true).find_all();
            CHECK_EQUAL(1 + i_1, const_subsubview_mix.size());
            for (int i_3 = 0; i_3 != 1 + i_1; ++i_3) {
                CHECK_EQUAL(true, bool(const_subsubview_mix.get_subtable(1, i_3)));
                CHECK_EQUAL(false, bool(const_subsubview_mix.get_subtable(2, i_3))); // Mixed
                CHECK_EQUAL(0, const_subsubview_mix.get_subtable_size(1, i_3));
                CHECK_EQUAL(0, const_subsubview_mix.get_subtable_size(2, i_3)); // Mixed
            }
        }
        for (int i_2 = 0; i_2 != 2 + i_1; ++i_2) {
            CHECK_EQUAL(true, bool(const_subview.get_subtable(1, i_2)));
            CHECK_EQUAL(i_2 == 1 + i_1, bool(const_subview.get_subtable(2, i_2))); // Mixed
            CHECK_EQUAL(i_2 == 0 + i_1 ? 2 * (3 + i_1) : 0, const_subview.get_subtable_size(1, i_2));
            CHECK_EQUAL(i_2 == 1 + i_1 ? 2 * (1 + i_1) : 0, const_subview.get_subtable_size(2, i_2)); // Mixed
        }

        ConstTableRef subtab_mix = const_view.get_subtable(2, i_1);
        ConstTableView const_subview_mix = subtab_mix->where().equal(0, true).find_all();
        CHECK_EQUAL(3 + i_1, const_subview_mix.size());
        {
            ConstTableRef subsubtab = const_subview_mix.get_subtable(1, 1 + i_1);
            ConstTableView const_subsubview = subsubtab->where().equal(0, true).find_all();
            CHECK_EQUAL(7 + i_1, const_subsubview.size());
            for (int i_3 = 0; i_3 != 7 + i_1; ++i_3) {
                CHECK_EQUAL(true, bool(const_subsubview.get_subtable(1, i_3)));
                CHECK_EQUAL(false, bool(const_subsubview.get_subtable(2, i_3))); // Mixed
                CHECK_EQUAL(0, const_subsubview.get_subtable_size(1, i_3));
                CHECK_EQUAL(0, const_subsubview.get_subtable_size(2, i_3)); // Mixed
            }

            ConstTableRef subsubtab_mix = const_subview_mix.get_subtable(2, 2 + i_1);
            ConstTableView const_subsubview_mix = subsubtab_mix->where().equal(0, true).find_all();
            CHECK_EQUAL(5 + i_1, const_subsubview_mix.size());
            for (int i_3 = 0; i_3 != 5 + i_1; ++i_3) {
                CHECK_EQUAL(true, bool(const_subsubview_mix.get_subtable(1, i_3)));
                CHECK_EQUAL(false, bool(const_subsubview_mix.get_subtable(2, i_3))); // Mixed
                CHECK_EQUAL(0, const_subsubview_mix.get_subtable_size(1, i_3));
                CHECK_EQUAL(0, const_subsubview_mix.get_subtable_size(2, i_3)); // Mixed
            }
        }
        for (int i_2 = 0; i_2 != 2 + i_1; ++i_2) {
            CHECK_EQUAL(true, bool(const_subview_mix.get_subtable(1, i_2)));
            CHECK_EQUAL(i_2 == 2 + i_1, bool(const_subview_mix.get_subtable(2, i_2))); // Mixed
            CHECK_EQUAL(i_2 == 1 + i_1 ? 2 * (7 + i_1) : 0, const_subview_mix.get_subtable_size(1, i_2));
            CHECK_EQUAL(i_2 == 2 + i_1 ? 2 * (5 + i_1) : 0, const_subview_mix.get_subtable_size(2, i_2)); // Mixed
        }

        CHECK_EQUAL(true, bool(const_view.get_subtable(1, i_1)));
        CHECK_EQUAL(true, bool(const_view.get_subtable(2, i_1))); // Mixed
        CHECK_EQUAL(2 * (2 + i_1), const_view.get_subtable_size(1, i_1));
        CHECK_EQUAL(2 * (3 + i_1), const_view.get_subtable_size(2, i_1)); // Mixed
    }
}


namespace {

REALM_TABLE_1(MyTable1, val, Int)

REALM_TABLE_2(MyTable2, val, Int, subtab, Subtable<MyTable1>)

REALM_TABLE_2(MyTable3, val, Int, subtab, Subtable<MyTable2>)

} // anonymous namespace

TEST(TableView_HighLevelSubtables)
{
    MyTable3 t;
    const MyTable3& ct = t;

    t.add();
    MyTable3::View v = t.column().val.find_all(0);
    MyTable3::ConstView cv = ct.column().val.find_all(0);

    {
        MyTable3::View v2 = v.column().val.find_all(0);
        MyTable3::ConstView cv2 = cv.column().val.find_all(0);

        MyTable3::ConstView cv3 = t.column().val.find_all(0);
        MyTable3::ConstView cv4 = v.column().val.find_all(0);

        // Also test assigment that converts to const
        cv3 = t.column().val.find_all(0);
        cv4 = v.column().val.find_all(0);

        static_cast<void>(v2);
        static_cast<void>(cv2);
        static_cast<void>(cv3);
        static_cast<void>(cv4);
    }

    {
        MyTable2::Ref s1 = v[0].subtab;
        MyTable2::ConstRef s2 = v[0].subtab;
        MyTable2::Ref s3 = v[0].subtab->get_table_ref();
        MyTable2::ConstRef s4 = v[0].subtab->get_table_ref();
        MyTable2::Ref s5 = v.column().subtab[0];
        MyTable2::ConstRef s6 = v.column().subtab[0];
        MyTable2::Ref s7 = v.column().subtab[0]->get_table_ref();
        MyTable2::ConstRef s8 = v.column().subtab[0]->get_table_ref();
        MyTable2::ConstRef cs1 = cv[0].subtab;
        MyTable2::ConstRef cs2 = cv[0].subtab->get_table_ref();
        MyTable2::ConstRef cs3 = cv.column().subtab[0];
        MyTable2::ConstRef cs4 = cv.column().subtab[0]->get_table_ref();
        static_cast<void>(s1);
        static_cast<void>(s2);
        static_cast<void>(s3);
        static_cast<void>(s4);
        static_cast<void>(s5);
        static_cast<void>(s6);
        static_cast<void>(s7);
        static_cast<void>(s8);
        static_cast<void>(cs1);
        static_cast<void>(cs2);
        static_cast<void>(cs3);
        static_cast<void>(cs4);
    }

    t[0].subtab->add();
    {
        MyTable1::Ref s1 = v[0].subtab[0].subtab;
        MyTable1::ConstRef s2 = v[0].subtab[0].subtab;
        MyTable1::Ref s3 = v[0].subtab[0].subtab->get_table_ref();
        MyTable1::ConstRef s4 = v[0].subtab[0].subtab->get_table_ref();
        MyTable1::Ref s5 = v.column().subtab[0]->column().subtab[0];
        MyTable1::ConstRef s6 = v.column().subtab[0]->column().subtab[0];
        MyTable1::Ref s7 = v.column().subtab[0]->column().subtab[0]->get_table_ref();
        MyTable1::ConstRef s8 = v.column().subtab[0]->column().subtab[0]->get_table_ref();
        MyTable1::ConstRef cs1 = cv[0].subtab[0].subtab;
        MyTable1::ConstRef cs2 = cv[0].subtab[0].subtab->get_table_ref();
        MyTable1::ConstRef cs3 = cv.column().subtab[0]->column().subtab[0];
        MyTable1::ConstRef cs4 = cv.column().subtab[0]->column().subtab[0]->get_table_ref();
        static_cast<void>(s1);
        static_cast<void>(s2);
        static_cast<void>(s3);
        static_cast<void>(s4);
        static_cast<void>(s5);
        static_cast<void>(s6);
        static_cast<void>(s7);
        static_cast<void>(s8);
        static_cast<void>(cs1);
        static_cast<void>(cs2);
        static_cast<void>(cs3);
        static_cast<void>(cs4);
    }

    v[0].subtab[0].val = 1;
    CHECK_EQUAL(v[0].subtab[0].val, 1);
    CHECK_EQUAL(v.column().subtab[0]->column().val[0], 1);
    CHECK_EQUAL(v[0].subtab->column().val[0], 1);
    CHECK_EQUAL(v.column().subtab[0][0].val, 1);

    v.column().subtab[0]->column().val[0] = 2;
    CHECK_EQUAL(v[0].subtab[0].val, 2);
    CHECK_EQUAL(v.column().subtab[0]->column().val[0], 2);
    CHECK_EQUAL(v[0].subtab->column().val[0], 2);
    CHECK_EQUAL(v.column().subtab[0][0].val, 2);

    v[0].subtab->column().val[0] = 3;
    CHECK_EQUAL(v[0].subtab[0].val, 3);
    CHECK_EQUAL(v.column().subtab[0]->column().val[0], 3);
    CHECK_EQUAL(v[0].subtab->column().val[0], 3);
    CHECK_EQUAL(v.column().subtab[0][0].val, 3);

    v.column().subtab[0][0].val = 4;
    CHECK_EQUAL(v[0].subtab[0].val, 4);
    CHECK_EQUAL(v.column().subtab[0]->column().val[0], 4);
    CHECK_EQUAL(v[0].subtab->column().val[0], 4);
    CHECK_EQUAL(v.column().subtab[0][0].val, 4);
    CHECK_EQUAL(cv[0].subtab[0].val, 4);
    CHECK_EQUAL(cv.column().subtab[0]->column().val[0], 4);
    CHECK_EQUAL(cv[0].subtab->column().val[0], 4);
    CHECK_EQUAL(cv.column().subtab[0][0].val, 4);

    v[0].subtab[0].subtab->add();
    v[0].subtab[0].subtab[0].val = 5;
    CHECK_EQUAL(v[0].subtab[0].subtab[0].val, 5);
    CHECK_EQUAL(v.column().subtab[0]->column().subtab[0]->column().val[0], 5);
    CHECK_EQUAL(cv[0].subtab[0].subtab[0].val, 5);
    CHECK_EQUAL(cv.column().subtab[0]->column().subtab[0]->column().val[0], 5);

    v.column().subtab[0]->column().subtab[0]->column().val[0] = 6;
    CHECK_EQUAL(v[0].subtab[0].subtab[0].val, 6);
    CHECK_EQUAL(v.column().subtab[0]->column().subtab[0]->column().val[0], 6);
    CHECK_EQUAL(cv[0].subtab[0].subtab[0].val, 6);
    CHECK_EQUAL(cv.column().subtab[0]->column().subtab[0]->column().val[0], 6);
}


TEST(TableView_ToString)
{
    TestTableInt2 tbl;

    tbl.add(2, 123456);
    tbl.add(4, 1234567);
    tbl.add(6, 12345678);
    tbl.add(4, 12345678);

    std::string s = "    first    second\n";
    std::string s0 = "0:      2    123456\n";
    std::string s1 = "1:      4   1234567\n";
    std::string s2 = "2:      6  12345678\n";
    std::string s3 = "3:      4  12345678\n";

    // Test full view
    std::stringstream ss;
    TestTableInt2::View tv = tbl.where().find_all();
    tv.to_string(ss);
    CHECK_EQUAL(s + s0 + s1 + s2 + s3, ss.str());

    // Find partial view: row 1+3
    std::stringstream ss2;
    tv = tbl.where().first.equal(4).find_all();
    tv.to_string(ss2);
    CHECK_EQUAL(s + s1 + s3, ss2.str());

    // test row_to_string. get row 0 of previous view - i.e. row 1 in tbl
    std::stringstream ss3;
    tv.row_to_string(0, ss3);
    CHECK_EQUAL(s + s1, ss3.str());
}


TEST(TableView_RefCounting)
{
    TableView tv, tv2;
    {
        TableRef t = Table::create();
        t->add_column(type_Int, "myint");
        t->add_empty_row();
        t->set_int(0, 0, 12);
        tv = t->where().find_all();
    }

    {
        TableRef t2 = Table::create();
        t2->add_column(type_String, "mystr");
        t2->add_empty_row();
        t2->set_string(0, 0, "just a test string");
        tv2 = t2->where().find_all();
    }

    // Now try to access TableView and see that the Table is still alive
    int64_t i = tv.get_int(0, 0);
    CHECK_EQUAL(i, 12);
    std::string s = tv2.get_string(0, 0);
    CHECK_EQUAL(s, "just a test string");
}


TEST(TableView_DynPivot)
{
    TableRef table = Table::create();
    size_t column_ndx_sex = table->add_column(type_String, "sex");
    size_t column_ndx_age = table->add_column(type_Int, "age");
    table->add_column(type_Bool, "hired");

    size_t count = 5000;
    for (size_t i = 0; i < count; ++i) {
        StringData sex = i % 2 ? "Male" : "Female";
        table->insert_empty_row(i);
        table->set_string(0, i, sex);
        table->set_int(1, i, 20 + (i % 20));
        table->set_bool(2, i, true);
    }

    TableView tv = table->where().find_all();

    Table result_count;
    tv.aggregate(0, 1, Table::aggr_count, result_count);
    int64_t half = count / 2;
    CHECK_EQUAL(2, result_count.get_column_count());
    CHECK_EQUAL(2, result_count.size());
    CHECK_EQUAL(half, result_count.get_int(1, 0));
    CHECK_EQUAL(half, result_count.get_int(1, 1));

    Table result_sum;
    tv.aggregate(column_ndx_sex, column_ndx_age, Table::aggr_sum, result_sum);

    Table result_avg;
    tv.aggregate(column_ndx_sex, column_ndx_age, Table::aggr_avg, result_avg);

    Table result_min;
    tv.aggregate(column_ndx_sex, column_ndx_age, Table::aggr_min, result_min);

    Table result_max;
    tv.aggregate(column_ndx_sex, column_ndx_age, Table::aggr_max, result_max);


    // Test with enumerated strings
    table->optimize();

    Table result_count2;
    tv.aggregate(column_ndx_sex, column_ndx_age, Table::aggr_count, result_count2);
    CHECK_EQUAL(2, result_count2.get_column_count());
    CHECK_EQUAL(2, result_count2.size());
    CHECK_EQUAL(half, result_count2.get_int(1, 0));
    CHECK_EQUAL(half, result_count2.get_int(1, 1));
}


TEST(TableView_RowAccessor)
{
    Table table;
    table.add_column(type_Int, "");
    table.add_empty_row();
    table.set_int(0, 0, 703);
    TableView tv = table.where().find_all();
    Row row = tv[0];
    CHECK_EQUAL(703, row.get_int(0));
    ConstRow crow = tv[0];
    CHECK_EQUAL(703, crow.get_int(0));
    ConstTableView ctv = table.where().find_all();
    ConstRow crow_2 = ctv[0];
    CHECK_EQUAL(703, crow_2.get_int(0));
}

TEST(TableView_FindBySourceNdx)
{
    Table table;
    table.add_column(type_Int, "");
    table.add_empty_row();
    table.add_empty_row();
    table.add_empty_row();
    table[0].set_int(0, 0);
    table[1].set_int(0, 1);
    table[2].set_int(0, 2);
    TableView tv = table.where().find_all();
    tv.sort(0, false);
    CHECK_EQUAL(0, tv.find_by_source_ndx(2));
    CHECK_EQUAL(1, tv.find_by_source_ndx(1));
    CHECK_EQUAL(2, tv.find_by_source_ndx(0));
}

TEST(TableView_MultiColSort)
{
    Table table;
    table.add_column(type_Int, "");
    table.add_column(type_Float, "");
    table.add_empty_row();
    table.add_empty_row();
    table.add_empty_row();
    table[0].set_int(0, 0);
    table[1].set_int(0, 1);
    table[2].set_int(0, 1);

    table[0].set_float(1, 0.f);
    table[1].set_float(1, 2.f);
    table[2].set_float(1, 1.f);

    TableView tv = table.where().find_all();

    std::vector<std::vector<size_t>> v = {{0}, {1}};
    std::vector<bool> a = {true, true};

    tv.sort(SortDescriptor{table, v, a});

    CHECK_EQUAL(tv.get_float(1, 0), 0.f);
    CHECK_EQUAL(tv.get_float(1, 1), 1.f);
    CHECK_EQUAL(tv.get_float(1, 2), 2.f);

    std::vector<bool> a_descending = {false, false};
    tv.sort(SortDescriptor{table, v, a_descending});

    CHECK_EQUAL(tv.get_float(1, 0), 2.f);
    CHECK_EQUAL(tv.get_float(1, 1), 1.f);
    CHECK_EQUAL(tv.get_float(1, 2), 0.f);

    std::vector<bool> a_ascdesc = {true, false};
    tv.sort(SortDescriptor{table, v, a_ascdesc});

    CHECK_EQUAL(tv.get_float(1, 0), 0.f);
    CHECK_EQUAL(tv.get_float(1, 1), 2.f);
    CHECK_EQUAL(tv.get_float(1, 2), 1.f);
}

TEST(TableView_QueryCopy)
{
    Table table;
    table.add_column(type_Int, "");
    table.add_empty_row();
    table.add_empty_row();
    table.add_empty_row();
    table[0].set_int(0, 0);
    table[1].set_int(0, 1);
    table[2].set_int(0, 2);

    // Test if copy-assign of Query in TableView works
    TableView tv = table.where().find_all();

    Query q = table.where();

    q.group();
    q.equal(0, 1);
    q.Or();
    q.equal(0, 2);
    q.end_group();

    q.count();

    Query q2;
    q2 = table.where().equal(0, 1234);

    q2 = q;
    size_t t = q2.count();

    CHECK_EQUAL(t, 2);
}

TEST(TableView_SortEnum)
{
    Table table;
    table.add_column(type_String, "str");
    table.add_empty_row(3);
    table[0].set_string(0, "foo");
    table[1].set_string(0, "foo");
    table[2].set_string(0, "foo");

    table.optimize();

    table.add_empty_row(3);
    table[3].set_string(0, "bbb");
    table[4].set_string(0, "aaa");
    table[5].set_string(0, "baz");

    TableView tv = table.where().find_all();
    tv.sort(0);

    CHECK_EQUAL(tv[0].get_string(0), "aaa");
    CHECK_EQUAL(tv[1].get_string(0), "baz");
    CHECK_EQUAL(tv[2].get_string(0), "bbb");
    CHECK_EQUAL(tv[3].get_string(0), "foo");
    CHECK_EQUAL(tv[4].get_string(0), "foo");
    CHECK_EQUAL(tv[5].get_string(0), "foo");
}


TEST(TableView_UnderlyingRowRemoval)
{
    struct Fixture {
        Table table;
        TableView view;
        Fixture()
        {
            table.add_column(type_Int, "a");
            table.add_column(type_Int, "b");
            table.add_empty_row(5);

            table.set_int(0, 0, 0);
            table.set_int(0, 1, 1);
            table.set_int(0, 2, 2);
            table.set_int(0, 3, 3);
            table.set_int(0, 4, 4);

            table.set_int(1, 0, 0);
            table.set_int(1, 1, 1);
            table.set_int(1, 2, 0);
            table.set_int(1, 3, 1);
            table.set_int(1, 4, 1);

            view = table.find_all_int(1, 0);
        }
    };

    // Sanity
    {
        Fixture f;
        CHECK_EQUAL(2, f.view.size());
        CHECK_EQUAL(0, f.view.get_source_ndx(0));
        CHECK_EQUAL(2, f.view.get_source_ndx(1));
    }

    // The following checks assume that unordered row removal in the underlying
    // table is done using `Table::move_last_over()`, and that Table::clear()
    // does that in reverse order of rows in the view.

    // Ordered remove()
    {
        Fixture f;
        f.view.remove(0);
        CHECK_EQUAL(4, f.table.size());
        CHECK_EQUAL(1, f.table.get_int(0, 0));
        CHECK_EQUAL(2, f.table.get_int(0, 1));
        CHECK_EQUAL(3, f.table.get_int(0, 2));
        CHECK_EQUAL(4, f.table.get_int(0, 3));
        CHECK_EQUAL(1, f.view.size());
        CHECK_EQUAL(1, f.view.get_source_ndx(0));
    }
    {
        Fixture f;
        f.view.remove(1);
        CHECK_EQUAL(4, f.table.size());
        CHECK_EQUAL(0, f.table.get_int(0, 0));
        CHECK_EQUAL(1, f.table.get_int(0, 1));
        CHECK_EQUAL(3, f.table.get_int(0, 2));
        CHECK_EQUAL(4, f.table.get_int(0, 3));
        CHECK_EQUAL(1, f.view.size());
        CHECK_EQUAL(0, f.view.get_source_ndx(0));
    }

    // Unordered remove()
    {
        Fixture f;
        f.view.remove(0, RemoveMode::unordered);
        CHECK_EQUAL(4, f.table.size());
        CHECK_EQUAL(4, f.table.get_int(0, 0));
        CHECK_EQUAL(1, f.table.get_int(0, 1));
        CHECK_EQUAL(2, f.table.get_int(0, 2));
        CHECK_EQUAL(3, f.table.get_int(0, 3));
        CHECK_EQUAL(1, f.view.size());
        CHECK_EQUAL(2, f.view.get_source_ndx(0));
    }
    {
        Fixture f;
        f.view.remove(1, RemoveMode::unordered);
        CHECK_EQUAL(4, f.table.size());
        CHECK_EQUAL(0, f.table.get_int(0, 0));
        CHECK_EQUAL(1, f.table.get_int(0, 1));
        CHECK_EQUAL(4, f.table.get_int(0, 2));
        CHECK_EQUAL(3, f.table.get_int(0, 3));
        CHECK_EQUAL(1, f.view.size());
        CHECK_EQUAL(0, f.view.get_source_ndx(0));
    }

    // Ordered remove_last()
    {
        Fixture f;
        f.view.remove_last();
        CHECK_EQUAL(4, f.table.size());
        CHECK_EQUAL(0, f.table.get_int(0, 0));
        CHECK_EQUAL(1, f.table.get_int(0, 1));
        CHECK_EQUAL(3, f.table.get_int(0, 2));
        CHECK_EQUAL(4, f.table.get_int(0, 3));
        CHECK_EQUAL(1, f.view.size());
        CHECK_EQUAL(0, f.view.get_source_ndx(0));
    }

    // Unordered remove_last()
    {
        Fixture f;
        f.view.remove_last(RemoveMode::unordered);
        CHECK_EQUAL(4, f.table.size());
        CHECK_EQUAL(0, f.table.get_int(0, 0));
        CHECK_EQUAL(1, f.table.get_int(0, 1));
        CHECK_EQUAL(4, f.table.get_int(0, 2));
        CHECK_EQUAL(3, f.table.get_int(0, 3));
        CHECK_EQUAL(1, f.view.size());
        CHECK_EQUAL(0, f.view.get_source_ndx(0));
    }

    // Ordered clear()
    {
        Fixture f;
        f.view.clear();
        CHECK_EQUAL(3, f.table.size());
        CHECK_EQUAL(1, f.table.get_int(0, 0));
        CHECK_EQUAL(3, f.table.get_int(0, 1));
        CHECK_EQUAL(4, f.table.get_int(0, 2));
        CHECK_EQUAL(0, f.view.size());
    }

    // Unordered clear()
    {
        Fixture f;
        f.view.clear(RemoveMode::unordered);
        CHECK_EQUAL(3, f.table.size());
        CHECK_EQUAL(3, f.table.get_int(0, 0));
        CHECK_EQUAL(1, f.table.get_int(0, 1));
        CHECK_EQUAL(4, f.table.get_int(0, 2));
        CHECK_EQUAL(0, f.view.size());
    }
}

TEST(TableView_Backlinks)
{
    Group group;

    TableRef source = group.add_table("source");
    source->add_column(type_Int, "int");

    TableRef links = group.add_table("links");
    links->add_column_link(type_Link, "link", *source);
    links->add_column_link(type_LinkList, "link_list", *source);

    source->add_empty_row(3);

    {
        // Links
        TableView tv = source->get_backlink_view(2, links.get(), 0);

        CHECK_EQUAL(tv.size(), 0);

        links->add_empty_row();
        links->set_link(0, 0, 2);

        tv.sync_if_needed();
        CHECK_EQUAL(tv.size(), 1);
        CHECK_EQUAL(tv[0].get_index(), links->get(0).get_index());
    }
    {
        // LinkViews
        TableView tv = source->get_backlink_view(2, links.get(), 1);

        CHECK_EQUAL(tv.size(), 0);

        auto ll = links->get_linklist(1, 0);
        ll->add(2);
        ll->add(0);
        ll->add(2);

        tv.sync_if_needed();
        CHECK_EQUAL(tv.size(), 2);
        CHECK_EQUAL(tv[0].get_index(), links->get(0).get_index());
    }
}

// Verify that a TableView that represents backlinks to a row functions correctly
// after being move-assigned.
TEST(TableView_BacklinksAfterMoveAssign)
{
    Group group;

    TableRef source = group.add_table("source");
    source->add_column(type_Int, "int");

    TableRef links = group.add_table("links");
    links->add_column_link(type_Link, "link", *source);
    links->add_column_link(type_LinkList, "link_list", *source);

    source->add_empty_row(3);

    {
        // Links
        TableView tv_source = source->get_backlink_view(2, links.get(), 0);
        TableView tv;
        tv = std::move(tv_source);

        CHECK_EQUAL(tv.size(), 0);

        links->add_empty_row();
        links->set_link(0, 0, 2);

        tv.sync_if_needed();
        CHECK_EQUAL(tv.size(), 1);
        CHECK_EQUAL(tv[0].get_index(), links->get(0).get_index());
    }
    {
        // LinkViews
        TableView tv_source = source->get_backlink_view(2, links.get(), 1);
        TableView tv;
        tv = std::move(tv_source);

        CHECK_EQUAL(tv.size(), 0);

        auto ll = links->get_linklist(1, 0);
        ll->add(2);
        ll->add(0);
        ll->add(2);

        tv.sync_if_needed();
        CHECK_EQUAL(tv.size(), 2);
        CHECK_EQUAL(tv[0].get_index(), links->get(0).get_index());
    }
}

// Verify that a TableView that represents backlinks continues to track the correct row
// when it moves within a table or is deleted.
TEST(TableView_BacklinksWhenTargetRowMovedOrDeleted)
{
    Group group;

    TableRef source = group.add_table("source");
    source->add_column(type_Int, "int");

    TableRef links = group.add_table("links");
    size_t col_link = links->add_column_link(type_Link, "link", *source);
    size_t col_linklist = links->add_column_link(type_LinkList, "link_list", *source);

    source->add_empty_row(3);

    links->add_empty_row(3);
    links->set_link(col_link, 0, 1);
    LinkViewRef ll = links->get_linklist(col_linklist, 0);
    ll->add(1);
    ll->add(0);

    links->set_link(col_link, 1, 1);
    ll = links->get_linklist(col_linklist, 1);
    ll->add(1);

    links->set_link(col_link, 2, 0);

    TableView tv_link = source->get_backlink_view(1, links.get(), col_link);
    TableView tv_linklist = source->get_backlink_view(1, links.get(), col_linklist);

    CHECK_EQUAL(tv_link.size(), 2);
    CHECK_EQUAL(tv_linklist.size(), 2);

    source->swap_rows(1, 0);
    tv_link.sync_if_needed();
    tv_linklist.sync_if_needed();

    CHECK_EQUAL(tv_link.size(), 2);
    CHECK_EQUAL(tv_linklist.size(), 2);

    CHECK(!tv_link.depends_on_deleted_object());
    CHECK(!tv_linklist.depends_on_deleted_object());

    source->move_last_over(0);

    CHECK(tv_link.depends_on_deleted_object());
    CHECK(tv_linklist.depends_on_deleted_object());

    CHECK(!tv_link.is_in_sync());
    CHECK(!tv_linklist.is_in_sync());

    tv_link.sync_if_needed();
    tv_linklist.sync_if_needed();

    CHECK(tv_link.is_in_sync());
    CHECK(tv_linklist.is_in_sync());

    CHECK_EQUAL(tv_link.size(), 0);
    CHECK_EQUAL(tv_linklist.size(), 0);

    source->add_empty_row();

    // TableViews that depend on a deleted row will stay in sync despite modifications to their table.
    CHECK(tv_link.is_in_sync());
    CHECK(tv_linklist.is_in_sync());
}

TEST(TableView_BacklinksWithColumnInsertion)
{
    Group g;
    TableRef target = g.add_table("target");
    target->add_column(type_Int, "int");
    target->add_empty_row(2);
    target->set_int(0, 1, 10);

    TableRef origin = g.add_table("origin");
    origin->add_column_link(type_Link, "link", *target);
    origin->add_column_link(type_LinkList, "linklist", *target);
    origin->add_empty_row(2);
    origin->set_link(0, 1, 1);
    origin->get_linklist(1, 1)->add(1);

    auto tv1 = target->get_backlink_view(1, origin.get(), 0);
    CHECK_EQUAL(tv1.size(), 1);
    CHECK_EQUAL(tv1.get_source_ndx(0), 1);

    auto tv2 = target->get_backlink_view(1, origin.get(), 1);
    CHECK_EQUAL(tv2.size(), 1);
    CHECK_EQUAL(tv1.get_source_ndx(0), 1);

    target->insert_column(0, type_String, "string");
    target->insert_empty_row(0);

    tv1.sync_if_needed();
    CHECK_EQUAL(tv1.size(), 1);
    CHECK_EQUAL(tv1.get_source_ndx(0), 1);

    tv2.sync_if_needed();
    CHECK_EQUAL(tv2.size(), 1);
    CHECK_EQUAL(tv2.get_source_ndx(0), 1);

    origin->insert_column(0, type_String, "string");
    target->insert_empty_row(0);
    origin->insert_empty_row(0);

    tv1.sync_if_needed();
    CHECK_EQUAL(tv1.size(), 1);
    CHECK_EQUAL(tv1.get_source_ndx(0), 2);

    tv2.sync_if_needed();
    CHECK_EQUAL(tv2.size(), 1);
    CHECK_EQUAL(tv2.get_source_ndx(0), 2);
}

namespace {
struct DistinctDirect {
    Table& table;
    DistinctDirect(TableRef, TableRef t)
        : table(*t)
    {
    }

    SortDescriptor operator()(std::initializer_list<size_t> columns, std::vector<bool> ascending = {}) const
    {
        std::vector<std::vector<size_t>> column_indices;
        for (size_t col : columns)
            column_indices.push_back({col});
        return SortDescriptor(table, column_indices, ascending);
    }

    size_t get_source_ndx(const TableView& tv, size_t ndx) const
    {
        return tv.get_source_ndx(ndx);
    }

    StringData get_string(const TableView& tv, size_t col, size_t row) const
    {
        return tv.get_string(col, row);
    }

    TableView find_all() const
    {
        return table.where().find_all();
    }
};

struct DistinctOverLink {
    Table& table;
    DistinctOverLink(TableRef t, TableRef)
        : table(*t)
    {
    }

    SortDescriptor operator()(std::initializer_list<size_t> columns, std::vector<bool> ascending = {}) const
    {
        std::vector<std::vector<size_t>> column_indices;
        for (size_t col : columns)
            column_indices.push_back({0, col});
        return SortDescriptor(table, column_indices, ascending);
    }

    size_t get_source_ndx(const TableView& tv, size_t ndx) const
    {
        return tv.get_link(0, ndx);
    }

    StringData get_string(const TableView& tv, size_t col, size_t row) const
    {
        return tv.get_link_target(0)->get_string(col, tv.get_link(0, row));
    }

    TableView find_all() const
    {
        return table.where().find_all();
    }
};
} // anonymous namespace

TEST_TYPES(TableView_Distinct, DistinctDirect, DistinctOverLink)
{
    // distinct() will preserve the original order of the row pointers, also if the order is a result of sort()
    // If multiple rows are indentical for the given set of distinct-columns, then only the first is kept.
    // You can call sync_if_needed() to update the distinct view, just like you can for a sorted view.
    // Each time you call distinct() it will first fetch the full original TableView contents and then apply
    // distinct() on that. So it distinct() does not filter the result of the previous distinct().

    // distinct() is internally based on the existing sort() method which is well tested. Hence it's not required
    // to test distinct() with all possible Realm data types.


    Group g;
    TableRef target = g.add_table("target");
    TableRef origin = g.add_table("origin");
    origin->add_column_link(type_Link, "link", *target);

    Table& t = *target;
    t.add_column(type_String, "s", true);
    t.add_column(type_Int, "i", true);
    t.add_column(type_Float, "f", true);

    t.add_empty_row(7);
    t.set_string(0, 0, StringData(""));
    t.set_int(1, 0, 100);
    t.set_float(2, 0, 100.);

    t.set_string(0, 1, realm::null());
    t.set_int(1, 1, 200);
    t.set_float(2, 1, 200.);

    t.set_string(0, 2, StringData(""));
    t.set_int(1, 2, 100);
    t.set_float(2, 2, 100.);

    t.set_string(0, 3, realm::null());
    t.set_int(1, 3, 200);
    t.set_float(2, 3, 200.);

    t.set_string(0, 4, "foo");
    t.set_int(1, 4, 300);
    t.set_float(2, 4, 300.);

    t.set_string(0, 5, "foo");
    t.set_int(1, 5, 400);
    t.set_float(2, 5, 400.);

    t.set_string(0, 6, "bar");
    t.set_int(1, 6, 500);
    t.set_float(2, 6, 500.);

    origin->add_empty_row(t.size());
    for (size_t i = 0; i < t.size(); ++i)
        origin->set_link(0, i, i);

    TEST_TYPE h(origin, target);

    TableView tv;
    tv = h.find_all();
    tv.distinct(h({0}));
    CHECK_EQUAL(tv.size(), 4);
    CHECK_EQUAL(h.get_source_ndx(tv, 0), 0);
    CHECK_EQUAL(h.get_source_ndx(tv, 1), 1);
    CHECK_EQUAL(h.get_source_ndx(tv, 2), 4);
    CHECK_EQUAL(h.get_source_ndx(tv, 3), 6);

    tv = h.find_all();
    tv.sort(h({0}));
    tv.distinct(h({0}));
    CHECK_EQUAL(tv.size(), 4);
    CHECK_EQUAL(h.get_source_ndx(tv, 0), 1);
    CHECK_EQUAL(h.get_source_ndx(tv, 1), 0);
    CHECK_EQUAL(h.get_source_ndx(tv, 2), 6);
    CHECK_EQUAL(h.get_source_ndx(tv, 3), 4);

    tv = h.find_all();
    tv.sort(h({0}, {false}));
    tv.distinct(h({0}));
    CHECK_EQUAL(h.get_source_ndx(tv, 0), 4);
    CHECK_EQUAL(h.get_source_ndx(tv, 1), 6);
    CHECK_EQUAL(h.get_source_ndx(tv, 2), 0);
    CHECK_EQUAL(h.get_source_ndx(tv, 3), 1);

    // Note here that our stable sort will sort the two "foo"s like row {4, 5}
    tv = h.find_all();
    tv.sort(h({0}, {false}));
    tv.distinct(h({0, 1}));
    CHECK_EQUAL(tv.size(), 5);
    CHECK_EQUAL(h.get_source_ndx(tv, 0), 4);
    CHECK_EQUAL(h.get_source_ndx(tv, 1), 5);
    CHECK_EQUAL(h.get_source_ndx(tv, 2), 6);
    CHECK_EQUAL(h.get_source_ndx(tv, 3), 0);
    CHECK_EQUAL(h.get_source_ndx(tv, 4), 1);


    // Now try distinct on string+float column. The float column has the same values as the int column
    // so the result should equal the test above
    tv = h.find_all();
    tv.sort(h({0}, {false}));
    tv.distinct(h({0, 1}));
    CHECK_EQUAL(tv.size(), 5);
    CHECK_EQUAL(h.get_source_ndx(tv, 0), 4);
    CHECK_EQUAL(h.get_source_ndx(tv, 1), 5);
    CHECK_EQUAL(h.get_source_ndx(tv, 2), 6);
    CHECK_EQUAL(h.get_source_ndx(tv, 3), 0);
    CHECK_EQUAL(h.get_source_ndx(tv, 4), 1);


    // Same as previous test, but with string column being Enum
    t.optimize(true); // true = enforce regardless if Realm thinks it pays off or not
    tv = h.find_all();
    tv.sort(h({0}, {false}));
    tv.distinct(h({0, 1}));
    CHECK_EQUAL(tv.size(), 5);
    CHECK_EQUAL(h.get_source_ndx(tv, 0), 4);
    CHECK_EQUAL(h.get_source_ndx(tv, 1), 5);
    CHECK_EQUAL(h.get_source_ndx(tv, 2), 6);
    CHECK_EQUAL(h.get_source_ndx(tv, 3), 0);
    CHECK_EQUAL(h.get_source_ndx(tv, 4), 1);


    // Now test sync_if_needed()
    tv = h.find_all();
    // "", null, "", null, "foo", "foo", "bar"

    tv.sort(h({0}, {false}));
    // "foo", "foo", "bar", "", "", null, null

    CHECK_EQUAL(tv.size(), 7);
    CHECK_EQUAL(h.get_string(tv, 0, 0), "foo");
    CHECK_EQUAL(h.get_string(tv, 0, 1), "foo");
    CHECK_EQUAL(h.get_string(tv, 0, 2), "bar");
    CHECK_EQUAL(h.get_string(tv, 0, 3), "");
    CHECK_EQUAL(h.get_string(tv, 0, 4), "");
    CHECK(h.get_string(tv, 0, 5).is_null());
    CHECK(h.get_string(tv, 0, 6).is_null());

    tv.distinct(h({0}));
    // "foo", "bar", "", null

    // remove "bar"
    origin->remove(6);
    target->remove(6);
    // access to tv undefined; may crash

    tv.sync_if_needed();
    // "foo", "", null

    CHECK_EQUAL(tv.size(), 3);
    CHECK_EQUAL(h.get_string(tv, 0, 0), "foo");
    CHECK_EQUAL(h.get_string(tv, 0, 1), "");
    CHECK(h.get_string(tv, 0, 2).is_null());

    // Remove distinct property by providing empty column list. Now TableView should look like it
    // did just after our last tv.sort(0, false) above, but after having executed table.remove(6)
    tv.distinct(SortDescriptor{});
    // "foo", "foo", "", "", null, null
    CHECK_EQUAL(tv.size(), 6);
    CHECK_EQUAL(h.get_string(tv, 0, 0), "foo");
    CHECK_EQUAL(h.get_string(tv, 0, 1), "foo");
    CHECK_EQUAL(h.get_string(tv, 0, 2), "");
    CHECK_EQUAL(h.get_string(tv, 0, 3), "");
    CHECK(h.get_string(tv, 0, 4).is_null());
    CHECK(h.get_string(tv, 0, 5).is_null());
}

TEST(TableView_DistinctOverNullLink)
{
    Group g;
    TableRef target = g.add_table("target");
    target->add_column(type_Int, "value");
    target->add_empty_row(2);
    target->set_int(0, 0, 1);
    target->set_int(0, 0, 2);

    TableRef origin = g.add_table("origin");
    origin->add_column_link(type_Link, "link", *target);
    origin->add_empty_row(5);
    origin->set_link(0, 0, 0);
    origin->set_link(0, 1, 1);
    origin->set_link(0, 2, 0);
    origin->set_link(0, 3, 1);
    // 4 is null

    auto tv = origin->where().find_all();
    tv.distinct(SortDescriptor(*origin, {{0, 0}}));
    CHECK_EQUAL(tv.size(), 2);
    CHECK_EQUAL(tv.get_source_ndx(0), 0);
    CHECK_EQUAL(tv.get_source_ndx(1), 1);
}

TEST(TableView_IsRowAttachedAfterClear)
{
    Table t;
    size_t col_id = t.add_column(type_Int, "id");

    t.add_empty_row(2);
    t.set_int(col_id, 0, 0);
    t.set_int(col_id, 1, 1);

    TableView tv = t.where().find_all();
    CHECK_EQUAL(2, tv.size());
    CHECK(tv.is_row_attached(0));
    CHECK(tv.is_row_attached(1));

    t.move_last_over(1);
    CHECK_EQUAL(2, tv.size());
    CHECK(tv.is_row_attached(0));
    CHECK(!tv.is_row_attached(1));

    t.clear();
    CHECK_EQUAL(2, tv.size());
    CHECK(!tv.is_row_attached(0));
    CHECK(!tv.is_row_attached(1));
}

TEST(TableView_IsInTableOrder)
{
    Group g;

    TableRef source = g.add_table("source");
    TableRef target = g.add_table("target");

    size_t col_link = source->add_column_link(type_LinkList, "link", *target);
    size_t col_name = source->add_column(type_String, "name");
    size_t col_id = target->add_column(type_Int, "id");
    target->add_search_index(col_id);

    source->add_empty_row();
    target->add_empty_row();

    // Detached views are in table order.
    TableView tv;
    CHECK_EQUAL(false, tv.is_in_table_order());

    // Queries not restricted by views are in table order.
    tv = target->where().find_all();
    CHECK_EQUAL(true, tv.is_in_table_order());

    // Views that have a distinct filter remain in table order.
    tv.distinct(col_id);
    CHECK_EQUAL(true, tv.is_in_table_order());

    // Views that are sorted are not guaranteed to be in table order.
    tv.sort(col_id, true);
    CHECK_EQUAL(false, tv.is_in_table_order());

    // Queries restricted by views are not guaranteed to be in table order.
    TableView restricting_view = target->where().equal(col_id, 0).find_all();
    tv = target->where(&restricting_view).find_all();
    CHECK_EQUAL(false, tv.is_in_table_order());

    // Backlinks are not guaranteed to be in table order.
    tv = target->get_backlink_view(0, source.get(), col_link);
    CHECK_EQUAL(false, tv.is_in_table_order());

    // Views derived from a LinkView are not guaranteed to be in table order.
    LinkViewRef ll = source->get_linklist(col_link, 0);
    tv = ll->get_sorted_view(col_name);
    CHECK_EQUAL(false, tv.is_in_table_order());

    // Views based directly on a table are in table order.
    tv = target->get_range_view(0, 1);
    CHECK_EQUAL(true, tv.is_in_table_order());
    tv = target->get_distinct_view(col_id);
    CHECK_EQUAL(true, tv.is_in_table_order());

    // … unless sorted.
    tv = target->get_sorted_view(col_id);
    CHECK_EQUAL(false, tv.is_in_table_order());
}


NONCONCURRENT_TEST(TableView_SortOrder_Similiar)
{
    TestTableString table;

    // This tests the expected sorting order with STRING_COMPARE_CORE_SIMILAR. See utf8_compare() in unicode.cpp. Only
    // characters
    // that have a visual representation are tested (control characters such as line feed are omitted).
    //
    // NOTE: Your editor must assume that Core source code is in utf8, and it must save as utf8, else this unit
    // test will fail.

    /*
    // This code snippet can be used to produce a list of *all* unicode characters in sorted order.
    //
    std::vector<int> original(collation_order, collation_order + sizeof collation_order / sizeof collation_order[0]);
    std::vector<int> sorted = original;
    std::sort(sorted.begin(), sorted.end());
    size_t highest_rank = sorted[sorted.size() - 1];

    std::wstring ws;
    for (size_t rank = 0; rank <= highest_rank; rank++) {
        size_t unicode = std::find(original.begin(), original.end(), rank) - original.begin();
        if (unicode != original.size()) {
            std::wcout << wchar_t(unicode) << "\n";
            std::cout << unicode << ", ";
            ws += wchar_t(unicode);
        }
    }
    */

    set_string_compare_method(STRING_COMPARE_CORE_SIMILAR, nullptr);

    table.add(" ");
    table.add("!");
    table.add("\"");
    table.add("#");
    table.add("%");
    table.add("&");
    table.add("'");
    table.add("(");
    table.add(")");
    table.add("*");
    table.add("+");
    table.add(",");
    table.add("-");
    table.add(".");
    table.add("/");
    table.add(":");
    table.add(";");
    table.add("<");
    table.add("=");
    table.add(">");
    table.add("?");
    table.add("@");
    table.add("[");
    table.add("\\");
    table.add("]");
    table.add("^");
    table.add("_");
    table.add("`");
    table.add("{");
    table.add("|");
    table.add("}");
    table.add("~");
    table.add(" ");
    table.add("¡");
    table.add("¦");
    table.add("§");
    table.add("¨");
    table.add("©");
    table.add("«");
    table.add("¬");
    table.add("®");
    table.add("¯");
    table.add("°");
    table.add("±");
    table.add("´");
    table.add("¶");
    table.add("·");
    table.add("¸");
    table.add("»");
    table.add("¿");
    table.add("×");
    table.add("÷");
    table.add("¤");
    table.add("¢");
    table.add("$");
    table.add("£");
    table.add("¥");
    table.add("0");
    table.add("1");
    table.add("¹");
    table.add("½");
    table.add("¼");
    table.add("2");
    table.add("²");
    table.add("3");
    table.add("³");
    table.add("¾");
    table.add("4");
    table.add("5");
    table.add("6");
    table.add("7");
    table.add("8");
    table.add("9");
    table.add("a");
    table.add("A");
    table.add("ª");
    table.add("á");
    table.add("Á");
    table.add("à");
    table.add("À");
    table.add("ă");
    table.add("Ă");
    table.add("â");
    table.add("Â");
    table.add("ǎ");
    table.add("Ǎ");
    table.add("å");
    table.add("Å");
    table.add("ǻ");
    table.add("Ǻ");
    table.add("ä");
    table.add("Ä");
    table.add("ǟ");
    table.add("Ǟ");
    table.add("ã");
    table.add("Ã");
    table.add("ȧ");
    table.add("Ȧ");
    table.add("ǡ");
    table.add("Ǡ");
    table.add("ą");
    table.add("Ą");
    table.add("ā");
    table.add("Ā");
    table.add("ȁ");
    table.add("Ȁ");
    table.add("ȃ");
    table.add("Ȃ");
    table.add("æ");
    table.add("Æ");
    table.add("ǽ");
    table.add("Ǽ");
    table.add("ǣ");
    table.add("Ǣ");
    table.add("Ⱥ");
    table.add("b");
    table.add("B");
    table.add("ƀ");
    table.add("Ƀ");
    table.add("Ɓ");
    table.add("ƃ");
    table.add("Ƃ");
    table.add("c");
    table.add("C");
    table.add("ć");
    table.add("Ć");
    table.add("ĉ");
    table.add("Ĉ");
    table.add("č");
    table.add("Č");
    table.add("ċ");
    table.add("Ċ");
    table.add("ç");
    table.add("Ç");
    table.add("ȼ");
    table.add("Ȼ");
    table.add("ƈ");
    table.add("Ƈ");
    table.add("d");
    table.add("D");
    table.add("ď");
    table.add("Ď");
    table.add("đ");
    table.add("Đ");
    table.add("ð");
    table.add("Ð");
    table.add("ȸ");
    table.add("ǳ");
    table.add("ǲ");
    table.add("Ǳ");
    table.add("ǆ");
    table.add("ǅ");
    table.add("Ǆ");
    table.add("Ɖ");
    table.add("Ɗ");
    table.add("ƌ");
    table.add("Ƌ");
    table.add("ȡ");
    table.add("e");
    table.add("E");
    table.add("é");
    table.add("É");
    table.add("è");
    table.add("È");
    table.add("ĕ");
    table.add("Ĕ");
    table.add("ê");
    table.add("Ê");
    table.add("ě");
    table.add("Ě");
    table.add("ë");
    table.add("Ë");
    table.add("ė");
    table.add("Ė");
    table.add("ȩ");
    table.add("Ȩ");
    table.add("ę");
    table.add("Ę");
    table.add("ē");
    table.add("Ē");
    table.add("ȅ");
    table.add("Ȅ");
    table.add("ȇ");
    table.add("Ȇ");
    table.add("ɇ");
    table.add("Ɇ");
    table.add("ǝ");
    table.add("Ǝ");
    table.add("Ə");
    table.add("Ɛ");
    table.add("f");
    table.add("F");
    table.add("ƒ");
    table.add("Ƒ");
    table.add("g");
    table.add("G");
    table.add("ǵ");
    table.add("Ǵ");
    table.add("ğ");
    table.add("Ğ");
    table.add("ĝ");
    table.add("Ĝ");
    table.add("ǧ");
    table.add("Ǧ");
    table.add("ġ");
    table.add("Ġ");
    table.add("ģ");
    table.add("Ģ");
    table.add("ǥ");
    table.add("Ǥ");
    table.add("Ɠ");
    table.add("Ɣ");
    table.add("ƣ");
    table.add("Ƣ");
    table.add("h");
    table.add("H");
    table.add("ĥ");
    table.add("Ĥ");
    table.add("ȟ");
    table.add("Ȟ");
    table.add("ħ");
    table.add("Ħ");
    table.add("ƕ");
    table.add("Ƕ");
    table.add("i");
    table.add("I");
    table.add("í");
    table.add("Í");
    table.add("ì");
    table.add("Ì");
    table.add("ĭ");
    table.add("Ĭ");
    table.add("î");
    table.add("Î");
    table.add("ǐ");
    table.add("Ǐ");
    table.add("ï");
    table.add("Ï");
    table.add("ĩ");
    table.add("Ĩ");
    table.add("İ");
    table.add("į");
    table.add("Į");
    table.add("ī");
    table.add("Ī");
    table.add("ȉ");
    table.add("Ȉ");
    table.add("ȋ");
    table.add("Ȋ");
    table.add("ĳ");
    table.add("Ĳ");
    table.add("ı");
    table.add("Ɨ");
    table.add("Ɩ");
    table.add("j");
    table.add("J");
    table.add("ĵ");
    table.add("Ĵ");
    table.add("ǰ");
    table.add("ȷ");
    table.add("ɉ");
    table.add("Ɉ");
    table.add("k");
    table.add("K");
    table.add("ǩ");
    table.add("Ǩ");
    table.add("ķ");
    table.add("Ķ");
    table.add("ƙ");
    table.add("Ƙ");
    table.add("ĺ");
    table.add("Ĺ");
    table.add("ľ");
    table.add("Ľ");
    table.add("ļ");
    table.add("Ļ");
    table.add("ł");
    table.add("Ł");
    table.add("ŀ");
    table.add("l");
    table.add("Ŀ");
    table.add("L");
    table.add("ǉ");
    table.add("ǈ");
    table.add("Ǉ");
    table.add("ƚ");
    table.add("Ƚ");
    table.add("ȴ");
    table.add("ƛ");
    table.add("m");
    table.add("M");
    table.add("n");
    table.add("N");
    table.add("ń");
    table.add("Ń");
    table.add("ǹ");
    table.add("Ǹ");
    table.add("ň");
    table.add("Ň");
    table.add("ñ");
    table.add("Ñ");
    table.add("ņ");
    table.add("Ņ");
    table.add("ǌ");
    table.add("ǋ");
    table.add("Ǌ");
    table.add("Ɲ");
    table.add("ƞ");
    table.add("Ƞ");
    table.add("ȵ");
    table.add("ŋ");
    table.add("Ŋ");
    table.add("o");
    table.add("O");
    table.add("º");
    table.add("ó");
    table.add("Ó");
    table.add("ò");
    table.add("Ò");
    table.add("ŏ");
    table.add("Ŏ");
    table.add("ô");
    table.add("Ô");
    table.add("ǒ");
    table.add("Ǒ");
    table.add("ö");
    table.add("Ö");
    table.add("ȫ");
    table.add("Ȫ");
    table.add("ő");
    table.add("Ő");
    table.add("õ");
    table.add("Õ");
    table.add("ȭ");
    table.add("Ȭ");
    table.add("ȯ");
    table.add("Ȯ");
    table.add("ȱ");
    table.add("Ȱ");
    table.add("ø");
    table.add("Ø");
    table.add("ǿ");
    table.add("Ǿ");
    table.add("ǫ");
    table.add("Ǫ");
    table.add("ǭ");
    table.add("Ǭ");
    table.add("ō");
    table.add("Ō");
    table.add("ȍ");
    table.add("Ȍ");
    table.add("ȏ");
    table.add("Ȏ");
    table.add("ơ");
    table.add("Ơ");
    table.add("œ");
    table.add("Œ");
    table.add("Ɔ");
    table.add("Ɵ");
    table.add("ȣ");
    table.add("Ȣ");
    table.add("p");
    table.add("P");
    table.add("ƥ");
    table.add("Ƥ");
    table.add("q");
    table.add("Q");
    table.add("ȹ");
    table.add("ɋ");
    table.add("Ɋ");
    table.add("ĸ");
    table.add("r");
    table.add("R");
    table.add("ŕ");
    table.add("Ŕ");
    table.add("ř");
    table.add("Ř");
    table.add("ŗ");
    table.add("Ŗ");
    table.add("ȑ");
    table.add("Ȑ");
    table.add("ȓ");
    table.add("Ȓ");
    table.add("Ʀ");
    table.add("ɍ");
    table.add("Ɍ");
    table.add("s");
    table.add("S");
    table.add("ś");
    table.add("Ś");
    table.add("ŝ");
    table.add("Ŝ");
    table.add("š");
    table.add("Š");
    table.add("ş");
    table.add("Ş");
    table.add("ș");
    table.add("Ș");
    table.add("ſ");
    table.add("ß");
    table.add("ȿ");
    table.add("Ʃ");
    table.add("ƪ");
    table.add("t");
    table.add("T");
    table.add("ť");
    table.add("Ť");
    table.add("ţ");
    table.add("Ţ");
    table.add("ț");
    table.add("Ț");
    table.add("ƾ");
    table.add("ŧ");
    table.add("Ŧ");
    table.add("Ⱦ");
    table.add("ƫ");
    table.add("ƭ");
    table.add("Ƭ");
    table.add("Ʈ");
    table.add("ȶ");
    table.add("u");
    table.add("U");
    table.add("ú");
    table.add("Ú");
    table.add("ù");
    table.add("Ù");
    table.add("ŭ");
    table.add("Ŭ");
    table.add("û");
    table.add("Û");
    table.add("ǔ");
    table.add("Ǔ");
    table.add("ů");
    table.add("Ů");
    table.add("ü");
    table.add("Ü");
    table.add("ǘ");
    table.add("Ǘ");
    table.add("ǜ");
    table.add("Ǜ");
    table.add("ǚ");
    table.add("Ǚ");
    table.add("ǖ");
    table.add("Ǖ");
    table.add("ű");
    table.add("Ű");
    table.add("ũ");
    table.add("Ũ");
    table.add("ų");
    table.add("Ų");
    table.add("ū");
    table.add("Ū");
    table.add("ȕ");
    table.add("Ȕ");
    table.add("ȗ");
    table.add("Ȗ");
    table.add("ư");
    table.add("Ư");
    table.add("Ʉ");
    table.add("Ɯ");
    table.add("Ʊ");
    table.add("v");
    table.add("V");
    table.add("Ʋ");
    table.add("Ʌ");
    table.add("w");
    table.add("W");
    table.add("ŵ");
    table.add("Ŵ");
    table.add("x");
    table.add("X");
    table.add("y");
    table.add("Y");
    table.add("ý");
    table.add("Ý");
    table.add("ŷ");
    table.add("Ŷ");
    table.add("ÿ");
    table.add("Ÿ");
    table.add("ȳ");
    table.add("Ȳ");
    table.add("ɏ");
    table.add("Ɏ");
    table.add("ƴ");
    table.add("Ƴ");
    table.add("ȝ");
    table.add("Ȝ");
    table.add("z");
    table.add("Z");
    table.add("ź");
    table.add("Ź");
    table.add("ž");
    table.add("Ž");
    table.add("ż");
    table.add("Ż");
    table.add("ƍ");
    table.add("ƶ");
    table.add("Ƶ");
    table.add("ȥ");
    table.add("Ȥ");
    table.add("ɀ");
    table.add("Ʒ");
    table.add("ǯ");
    table.add("Ǯ");
    table.add("ƹ");
    table.add("Ƹ");
    table.add("ƺ");
    table.add("þ");
    table.add("Þ");
    table.add("ƿ");
    table.add("Ƿ");
    table.add("ƻ");
    table.add("ƨ");
    table.add("Ƨ");
    table.add("ƽ");
    table.add("Ƽ");
    table.add("ƅ");
    table.add("Ƅ");
    table.add("ɂ");
    table.add("Ɂ");
    table.add("ŉ");
    table.add("ǀ");
    table.add("ǁ");
    table.add("ǂ");
    table.add("ǃ");
    table.add("µ");

    // Core-only is default comparer
    TestTableString::View v1 = table.where().find_all();
    TestTableString::View v2 = table.where().find_all();

    v2.column().first.sort();

    for (size_t t = 0; t < v1.size(); t++) {
        CHECK_EQUAL(v1.get_source_ndx(t), v2.get_source_ndx(t));
    }

    // Set back to default in case other tests rely on this
    set_string_compare_method(STRING_COMPARE_CORE, nullptr);
}


NONCONCURRENT_TEST(TableView_SortOrder_Core)
{
    TestTableString table;

    // This tests the expected sorting order with STRING_COMPARE_CORE. See utf8_compare() in unicode.cpp. Only
    // characters
    // that have a visual representation are tested (control characters such as line feed are omitted).
    //
    // NOTE: Your editor must assume that Core source code is in utf8, and it must save as utf8, else this unit
    // test will fail.

    set_string_compare_method(STRING_COMPARE_CORE, nullptr);

    table.add("'");
    table.add("-");
    table.add(" ");
    table.add(" ");
    table.add("!");
    table.add("\"");
    table.add("#");
    table.add("$");
    table.add("%");
    table.add("&");
    table.add("(");
    table.add(")");
    table.add("*");
    table.add(",");
    table.add(".");
    table.add("/");
    table.add(":");
    table.add(";");
    table.add("?");
    table.add("@");
    table.add("[");
    table.add("\\");
    table.add("^");
    table.add("_");
    table.add("`");
    table.add("{");
    table.add("|");
    table.add("}");
    table.add("~");
    table.add("¡");
    table.add("¦");
    table.add("¨");
    table.add("¯");
    table.add("´");
    table.add("¸");
    table.add("¿");
    table.add("ǃ");
    table.add("¢");
    table.add("£");
    table.add("¤");
    table.add("¥");
    table.add("+");
    table.add("<");
    table.add("=");
    table.add(">");
    table.add("±");
    table.add("«");
    table.add("»");
    table.add("×");
    table.add("÷");
    table.add("ǀ");
    table.add("ǁ");
    table.add("ǂ");
    table.add("§");
    table.add("©");
    table.add("¬");
    table.add("®");
    table.add("°");
    table.add("µ");
    table.add("¶");
    table.add("·");
    table.add("0");
    table.add("¼");
    table.add("½");
    table.add("¾");
    table.add("1");
    table.add("¹");
    table.add("2");
    table.add("ƻ");
    table.add("²");
    table.add("3");
    table.add("³");
    table.add("4");
    table.add("5");
    table.add("ƽ");
    table.add("Ƽ");
    table.add("6");
    table.add("7");
    table.add("8");
    table.add("9");
    table.add("a");
    table.add("A");
    table.add("ª");
    table.add("á");
    table.add("Á");
    table.add("à");
    table.add("À");
    table.add("ȧ");
    table.add("Ȧ");
    table.add("â");
    table.add("Â");
    table.add("ǎ");
    table.add("Ǎ");
    table.add("ă");
    table.add("Ă");
    table.add("ā");
    table.add("Ā");
    table.add("ã");
    table.add("Ã");
    table.add("ą");
    table.add("Ą");
    table.add("Ⱥ");
    table.add("ǡ");
    table.add("Ǡ");
    table.add("ǻ");
    table.add("Ǻ");
    table.add("ǟ");
    table.add("Ǟ");
    table.add("ȁ");
    table.add("Ȁ");
    table.add("ȃ");
    table.add("Ȃ");
    table.add("ǽ");
    table.add("Ǽ");
    table.add("b");
    table.add("B");
    table.add("ƀ");
    table.add("Ƀ");
    table.add("Ɓ");
    table.add("ƃ");
    table.add("Ƃ");
    table.add("ƅ");
    table.add("Ƅ");
    table.add("c");
    table.add("C");
    table.add("ć");
    table.add("Ć");
    table.add("ċ");
    table.add("Ċ");
    table.add("ĉ");
    table.add("Ĉ");
    table.add("č");
    table.add("Č");
    table.add("ç");
    table.add("Ç");
    table.add("ȼ");
    table.add("Ȼ");
    table.add("ƈ");
    table.add("Ƈ");
    table.add("Ɔ");
    table.add("d");
    table.add("D");
    table.add("ď");
    table.add("Ď");
    table.add("đ");
    table.add("Đ");
    table.add("ƌ");
    table.add("Ƌ");
    table.add("Ɗ");
    table.add("ð");
    table.add("Ð");
    table.add("ƍ");
    table.add("ȸ");
    table.add("ǳ");
    table.add("ǲ");
    table.add("Ǳ");
    table.add("ǆ");
    table.add("ǅ");
    table.add("Ǆ");
    table.add("Ɖ");
    table.add("ȡ");
    table.add("e");
    table.add("E");
    table.add("é");
    table.add("É");
    table.add("è");
    table.add("È");
    table.add("ė");
    table.add("Ė");
    table.add("ê");
    table.add("Ê");
    table.add("ë");
    table.add("Ë");
    table.add("ě");
    table.add("Ě");
    table.add("ĕ");
    table.add("Ĕ");
    table.add("ē");
    table.add("Ē");
    table.add("ę");
    table.add("Ę");
    table.add("ȩ");
    table.add("Ȩ");
    table.add("ɇ");
    table.add("Ɇ");
    table.add("ȅ");
    table.add("Ȅ");
    table.add("ȇ");
    table.add("Ȇ");
    table.add("ǝ");
    table.add("Ǝ");
    table.add("Ə");
    table.add("Ɛ");
    table.add("ȝ");
    table.add("Ȝ");
    table.add("f");
    table.add("F");
    table.add("ƒ");
    table.add("Ƒ");
    table.add("g");
    table.add("G");
    table.add("ǵ");
    table.add("Ǵ");
    table.add("ġ");
    table.add("Ġ");
    table.add("ĝ");
    table.add("Ĝ");
    table.add("ǧ");
    table.add("Ǧ");
    table.add("ğ");
    table.add("Ğ");
    table.add("ģ");
    table.add("Ģ");
    table.add("ǥ");
    table.add("Ǥ");
    table.add("Ɠ");
    table.add("Ɣ");
    table.add("h");
    table.add("H");
    table.add("ĥ");
    table.add("Ĥ");
    table.add("ȟ");
    table.add("Ȟ");
    table.add("ħ");
    table.add("Ħ");
    table.add("ƕ");
    table.add("Ƕ");
    table.add("i");
    table.add("I");
    table.add("ı");
    table.add("í");
    table.add("Í");
    table.add("ì");
    table.add("Ì");
    table.add("İ");
    table.add("î");
    table.add("Î");
    table.add("ï");
    table.add("Ï");
    table.add("ǐ");
    table.add("Ǐ");
    table.add("ĭ");
    table.add("Ĭ");
    table.add("ī");
    table.add("Ī");
    table.add("ĩ");
    table.add("Ĩ");
    table.add("į");
    table.add("Į");
    table.add("Ɨ");
    table.add("ȉ");
    table.add("Ȉ");
    table.add("ȋ");
    table.add("Ȋ");
    table.add("Ɩ");
    table.add("ĳ");
    table.add("Ĳ");
    table.add("j");
    table.add("J");
    table.add("ȷ");
    table.add("ĵ");
    table.add("Ĵ");
    table.add("ǰ");
    table.add("ɉ");
    table.add("Ɉ");
    table.add("k");
    table.add("K");
    table.add("ǩ");
    table.add("Ǩ");
    table.add("ķ");
    table.add("Ķ");
    table.add("ƙ");
    table.add("Ƙ");
    table.add("l");
    table.add("L");
    table.add("ĺ");
    table.add("Ĺ");
    table.add("ŀ");
    table.add("Ŀ");
    table.add("ľ");
    table.add("Ľ");
    table.add("ļ");
    table.add("Ļ");
    table.add("ƚ");
    table.add("Ƚ");
    table.add("ł");
    table.add("Ł");
    table.add("ƛ");
    table.add("ǉ");
    table.add("ǈ");
    table.add("Ǉ");
    table.add("ȴ");
    table.add("m");
    table.add("M");
    table.add("Ɯ");
    table.add("n");
    table.add("N");
    table.add("ń");
    table.add("Ń");
    table.add("ǹ");
    table.add("Ǹ");
    table.add("ň");
    table.add("Ň");
    table.add("ñ");
    table.add("Ñ");
    table.add("ņ");
    table.add("Ņ");
    table.add("Ɲ");
    table.add("ŉ");
    table.add("ƞ");
    table.add("Ƞ");
    table.add("ǌ");
    table.add("ǋ");
    table.add("Ǌ");
    table.add("ȵ");
    table.add("ŋ");
    table.add("Ŋ");
    table.add("o");
    table.add("O");
    table.add("º");
    table.add("ó");
    table.add("Ó");
    table.add("ò");
    table.add("Ò");
    table.add("ȯ");
    table.add("Ȯ");
    table.add("ô");
    table.add("Ô");
    table.add("ǒ");
    table.add("Ǒ");
    table.add("ŏ");
    table.add("Ŏ");
    table.add("ō");
    table.add("Ō");
    table.add("õ");
    table.add("Õ");
    table.add("ǫ");
    table.add("Ǫ");
    table.add("Ɵ");
    table.add("ȱ");
    table.add("Ȱ");
    table.add("ȫ");
    table.add("Ȫ");
    table.add("ǿ");
    table.add("Ǿ");
    table.add("ȭ");
    table.add("Ȭ");
    table.add("ǭ");
    table.add("Ǭ");
    table.add("ȍ");
    table.add("Ȍ");
    table.add("ȏ");
    table.add("Ȏ");
    table.add("ơ");
    table.add("Ơ");
    table.add("ƣ");
    table.add("Ƣ");
    table.add("œ");
    table.add("Œ");
    table.add("ȣ");
    table.add("Ȣ");
    table.add("p");
    table.add("P");
    table.add("ƥ");
    table.add("Ƥ");
    table.add("q");
    table.add("Q");
    table.add("ĸ");
    table.add("ɋ");
    table.add("Ɋ");
    table.add("ȹ");
    table.add("r");
    table.add("R");
    table.add("Ʀ");
    table.add("ŕ");
    table.add("Ŕ");
    table.add("ř");
    table.add("Ř");
    table.add("ŗ");
    table.add("Ŗ");
    table.add("ɍ");
    table.add("Ɍ");
    table.add("ȑ");
    table.add("Ȑ");
    table.add("ȓ");
    table.add("Ȓ");
    table.add("s");
    table.add("S");
    table.add("ś");
    table.add("Ś");
    table.add("ŝ");
    table.add("Ŝ");
    table.add("š");
    table.add("Š");
    table.add("ş");
    table.add("Ş");
    table.add("ș");
    table.add("Ș");
    table.add("ȿ");
    table.add("Ʃ");
    table.add("ƨ");
    table.add("Ƨ");
    table.add("ƪ");
    table.add("ß");
    table.add("ſ");
    table.add("t");
    table.add("T");
    table.add("ť");
    table.add("Ť");
    table.add("ţ");
    table.add("Ţ");
    table.add("ƭ");
    table.add("Ƭ");
    table.add("ƫ");
    table.add("Ʈ");
    table.add("ț");
    table.add("Ț");
    table.add("Ⱦ");
    table.add("ȶ");
    table.add("þ");
    table.add("Þ");
    table.add("ŧ");
    table.add("Ŧ");
    table.add("u");
    table.add("U");
    table.add("ú");
    table.add("Ú");
    table.add("ù");
    table.add("Ù");
    table.add("û");
    table.add("Û");
    table.add("ǔ");
    table.add("Ǔ");
    table.add("ŭ");
    table.add("Ŭ");
    table.add("ū");
    table.add("Ū");
    table.add("ũ");
    table.add("Ũ");
    table.add("ů");
    table.add("Ů");
    table.add("ų");
    table.add("Ų");
    table.add("Ʉ");
    table.add("ǘ");
    table.add("Ǘ");
    table.add("ǜ");
    table.add("Ǜ");
    table.add("ǚ");
    table.add("Ǚ");
    table.add("ǖ");
    table.add("Ǖ");
    table.add("ȕ");
    table.add("Ȕ");
    table.add("ȗ");
    table.add("Ȗ");
    table.add("ư");
    table.add("Ư");
    table.add("Ʊ");
    table.add("v");
    table.add("V");
    table.add("Ʋ");
    table.add("Ʌ");
    table.add("w");
    table.add("W");
    table.add("ŵ");
    table.add("Ŵ");
    table.add("ƿ");
    table.add("Ƿ");
    table.add("x");
    table.add("X");
    table.add("y");
    table.add("Y");
    table.add("ý");
    table.add("Ý");
    table.add("ŷ");
    table.add("Ŷ");
    table.add("ÿ");
    table.add("Ÿ");
    table.add("ȳ");
    table.add("Ȳ");
    table.add("ű");
    table.add("Ű");
    table.add("ɏ");
    table.add("Ɏ");
    table.add("ƴ");
    table.add("Ƴ");
    table.add("ü");
    table.add("Ü");
    table.add("z");
    table.add("Z");
    table.add("ź");
    table.add("Ź");
    table.add("ż");
    table.add("Ż");
    table.add("ž");
    table.add("Ž");
    table.add("ƶ");
    table.add("Ƶ");
    table.add("ȥ");
    table.add("Ȥ");
    table.add("ɀ");
    table.add("æ");
    table.add("Æ");
    table.add("Ʒ");
    table.add("ǣ");
    table.add("Ǣ");
    table.add("ä");
    table.add("Ä");
    table.add("ǯ");
    table.add("Ǯ");
    table.add("ƹ");
    table.add("Ƹ");
    table.add("ƺ");
    table.add("ø");
    table.add("Ø");
    table.add("ö");
    table.add("Ö");
    table.add("ő");
    table.add("Ő");
    table.add("å");
    table.add("Å");
    table.add("ƾ");
    table.add("ɂ");
    table.add("Ɂ");

    // Core-only is default comparer
    TestTableString::View v1 = table.where().find_all();
    TestTableString::View v2 = table.where().find_all();

    v2.column().first.sort();

    for (size_t t = 0; t < v1.size(); t++) {
        CHECK_EQUAL(v1.get_source_ndx(t), v2.get_source_ndx(t));
    }

    // Set back to default in case other tests rely on this
    set_string_compare_method(STRING_COMPARE_CORE, nullptr);
}


// Verify that copy-constructed and copy-assigned TableViews work normally.
TEST(TableView_Copy)
{
    Table table;
    size_t col_id = table.add_column(type_Int, "id");
    for (size_t i = 0; i < 3; ++i)
        table.set_int(col_id, table.add_empty_row(), i);

    TableView tv = (table.column<Int>(col_id) > 0).find_all();
    CHECK_EQUAL(2, tv.size());

    TableView copy_1(tv);
    TableView copy_2;
    copy_2 = tv;

    CHECK_EQUAL(2, copy_1.size());
    CHECK_EQUAL(1, copy_1.get_source_ndx(0));
    CHECK_EQUAL(2, copy_1.get_source_ndx(1));

    CHECK_EQUAL(2, copy_2.size());
    CHECK_EQUAL(1, copy_2.get_source_ndx(0));
    CHECK_EQUAL(2, copy_2.get_source_ndx(1));

    table.move_last_over(1);

    CHECK(!copy_1.is_in_sync());
    CHECK(!copy_2.is_in_sync());

    copy_1.sync_if_needed();
    CHECK_EQUAL(1, copy_1.size());
    CHECK_EQUAL(1, copy_1.get_source_ndx(0));

    copy_2.sync_if_needed();
    CHECK_EQUAL(1, copy_2.size());
    CHECK_EQUAL(1, copy_2.get_source_ndx(0));
}

TEST(TableView_InsertColumnsAfterSort)
{
    Table table;
    table.add_column(type_Int, "value");
    table.add_empty_row(10);
    for (size_t i = 0; i < 10; ++i)
        table.set_int(0, i, i);

    SortDescriptor desc(table, {{0}}, {false}); // sort by the one column in descending order

    table.insert_column(0, type_String, "0");
    auto tv = table.get_sorted_view(desc);
    CHECK_EQUAL(tv.get_int(1, 0), 9);
    CHECK_EQUAL(tv.get_int(1, 9), 0);

    table.insert_column(0, type_String, "1");
    table.add_empty_row();
    tv.sync_if_needed();
    CHECK_EQUAL(tv.get_int(2, 0), 9);
    CHECK_EQUAL(tv.get_int(2, 10), 0);
}

#endif // TEST_TABLE_VIEW
