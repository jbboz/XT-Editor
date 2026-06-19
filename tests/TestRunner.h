#pragma once
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

struct TestRunner {
    struct Test { std::string name; std::function<void()> fn; };
    std::vector<Test> tests;
    int failed = 0;

    void add(const char* name, std::function<void()> fn) {
        tests.push_back({name, std::move(fn)});
    }

    int run() {
        int passed = 0;
        for (auto& t : tests) {
            try {
                t.fn();
                std::printf("  PASS  %s\n", t.name.c_str());
                ++passed;
            } catch (const std::exception& e) {
                std::printf("  FAIL  %s\n        %s\n", t.name.c_str(), e.what());
                ++failed;
            }
        }
        std::printf("\n%d passed, %d failed\n", passed, failed);
        return failed > 0 ? 1 : 0;
    }
};

#define EXPECT(cond) \
    do { if (!(cond)) throw std::runtime_error( \
        "EXPECT failed: " #cond " at line " + std::to_string(__LINE__)); } while(0)

#define EXPECT_EQ(a, b) \
    do { if (!((a) == (b))) throw std::runtime_error( \
        "EXPECT_EQ failed: " #a " == " #b " at line " + std::to_string(__LINE__)); } while(0)
