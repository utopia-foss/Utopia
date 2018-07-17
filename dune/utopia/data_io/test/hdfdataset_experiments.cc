#include "../hdfdataset.hh"
#include "../hdffile.hh"
#include "../hdfgroup.hh"
#include <chrono>
#include <iostream>
#include <thread>
using namespace std::literals::chrono_literals;
using namespace Utopia::DataIO;
// used for testing rvalue container returning adaptors
struct Point
{
    double x;
    double y;
    double z;
};

void write()
{
    // make data used later

    std::array<int, 4> arr{{0, 1, 2, 3}};
    std::array<int, 4> arr2{{4, 5, 6, 7}};

    std::shared_ptr<double> ptr(new double[5]);
    for (std::size_t i = 0; i < 5; ++i)
    {
        ptr.get()[i] = 3.14;
    }

    std::vector<Point> points(100);
    for (int i = 0; i < 100; ++i)
    {
        points[i].x = 3.14;
        points[i].y = 3.14 + 1;
        points[i].z = 3.14 + 2;
    }

    // make hdfobjects needed
    HDFFile file("testfile.h5", "w");
    auto contset = file.open_dataset("/containerdataset", {100}, {5});
    auto contcontset = file.open_dataset("/containercontainerdataset", {100}, {5});
    auto stringset = file.open_dataset("/stringdataset", {100}, {5});
    auto ptrset = file.open_dataset("/pointerdataset", {100}, {5});
    auto scalarset = file.open_dataset("/scalardataset", {100}, {5});
    auto twoDdataset = file.open_dataset("/2ddataset", {10, 100}, {1, 5});
    auto adapteddataset = file.open_dataset("/adapteddataset", {500}, {50});
    auto largedataset1 = file.open_dataset("/largedataset1", {400000});
    auto largedataset2 = file.open_dataset("/largedataset2", {400000});
    auto largedataset3 = file.open_dataset("/largedataset3", {400000});

    contset->write(std::vector<double>(10, 3.14));
    contset->write(std::vector<double>(10, 6.28));
    contset->write(std::vector<double>(10, 9.42));

    contcontset->write(std::vector<std::array<int, 4>>(20, arr));
    contcontset->write(std::vector<std::array<int, 4>>(20, arr2));

    stringset->write(std::string("niggastring"));

    for (std::size_t i = 0; i < 25; ++i)
    {
        stringset->write(std::to_string(i));
    }

    ptrset->write(ptr.get(), {5});

    for (std::size_t j = 2; j < 4; ++j)
    {
        for (std::size_t i = 0; i < 5; ++i)
        {
            ptr.get()[i] = j * 3.14;
        }
        ptrset->write(ptr.get(), {5});
    }

    for (int i = 0; i < 5; ++i)
    {
        scalarset->write(i);
    }

    for (std::size_t i = 0; i < 6; ++i)
    {
        twoDdataset->write(std::vector<double>(100, i));
    }

    adapteddataset->write(points.begin(), points.end(),
                          [](auto& pt) { return pt.x; });

    adapteddataset->write(points.begin(), points.end(),
                          [](auto& pt) { return pt.y; });

    adapteddataset->write(points.begin(), points.end(),
                          [](auto& pt) { return pt.z; });

    std::vector<double> largevec(400000, 3.1415);

    std::chrono::high_resolution_clock clock;

    auto start = clock.now();
    largedataset3->write(largevec.begin(), largevec.end(),
                         [](auto& val) { return 3 * val; });
    auto end = clock.now();
    std::cout << "iterators: "
              << std::chrono::duration_cast<std::chrono::duration<double>>(end - start)
                     .count()
              << std::endl;

    start = clock.now();
    largedataset1->write(largevec);
    end = clock.now();
    std::cout << "lvalue: "
              << std::chrono::duration_cast<std::chrono::duration<double>>(end - start)
                     .count()
              << std::endl;

    start = clock.now();
    largedataset2->write(std::move(largevec));
    end = clock.now();
    std::cout << "rvalue: "
              << std::chrono::duration_cast<std::chrono::duration<double>>(end - start)
                     .count()
              << std::endl;
}

void read()
{
    // std::this_thread::sleep_for(30s);
    HDFFile file("testfile.h5", "r");
    auto contset = file.open_dataset("/containerdataset");
    auto contcontset = file.open_dataset("/containercontainerdataset");
    auto stringset = file.open_dataset("/stringdataset");
    auto ptrset = file.open_dataset("/pointerdataset");
    auto scalarset = file.open_dataset("/scalardataset");
    auto twoDdataset = file.open_dataset("/2ddataset");
    auto adapteddataset = file.open_dataset("/adapteddataset");

    std::cout << "contdataset" << std::endl;
    auto [shape, data] = contset->read<std::vector<double>>();
    std::cout << shape << std::endl;
    std::cout << data << std::endl;

    std::cout << "contcontdataset" << std::endl;
    auto [shape2, data2] = contcontset->read<std::vector<std::array<int, 4>>>();

    std::cout << shape2 << std::endl;
    std::cout << data2 << std::endl;

    std::cout << "stringdataset" << std::endl;
    auto [shape3, data3] = stringset->read<std::vector<std::string>>();
    for (auto& str : data3)
    {
        if (str != "")
        {
            std::cout << str << ",";
        }
    }
    std::cout << std::endl;

    std::cout << "stringdataset one string" << std::endl;
    auto [shape4, data4] = stringset->read<std::string>();
    std::cout << data4 << std::endl;

    auto [shape5, data5] = contset->read<std::vector<double>>({5}, {25}, {2});
    std::cout << "partial read from contset" << std::endl;
    std::cout << shape5 << std::endl;
    std::cout << data5 << std::endl;

    auto [shape6, data6] =
        twoDdataset->read<std::vector<double>>({2, 0}, {4, 100}, {1, 2});
    std::cout << "complete read from 2dset" << std::endl;
    std::cout << shape6 << std::endl;
    std::cout << data6 << std::endl;
}
int main()
{
    // std::this_thread::sleep_for(30s);
    write();
    read();
    return 0;
}