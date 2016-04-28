#include <realm.hpp>

#include "benchmark.hpp"
#include "results.hpp"
#include "random.hpp"

using namespace realm;
using namespace realm::test_util;

class WithNullColumn_Add1000EmptyRows :
    public AddEmptyRows<WithOneColumn<type_Timestamp, true>, 1000> {

    const char *name() const {
        return "WithNullColumn_Add1000EmptyRows";
    }
};

template<class WithClass, size_t N>
class AddRandomRows : public WithClass {

    Timestamp ts[N];

    void before_all(SharedGroup& sg)
    {
        Random random;
        int_fast64_t sinceEpoch;

        for (size_t i = 0; i < N; i++) {
            sinceEpoch = random.draw_int<int_fast64_t>();
            ts[i] = Timestamp(sinceEpoch, 0);
        }

        WithClass::before_all(sg);
    }

    void operator()(SharedGroup& sg)
    {
        WriteTransaction tr(sg);
        TableRef t = tr.get_table(0);
        t->add_empty_row(N);

        for (size_t i = 0; i < N; i++) {
            t->set_timestamp(0, i, ts[i]);
        }

        tr.commit();
    }
};

class WithNullColumn_Add1000RandomRows :
    public AddRandomRows<WithOneColumn<type_Timestamp, true>, 1000> {

    const char *name() const {
        return "WithNullColumn_Add1000RandomRows";
    }
};

template<class Benchmark>
void run(Results& results) {
    Benchmark benchmark;
    benchmark.run(results);
}

int main() {
    Results results(10);
    bench<WithNullColumn_Add1000EmptyRows>(results);
    bench<WithNullColumn_Add1000RandomRows>(results);
}
