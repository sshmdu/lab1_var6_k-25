// Compiler: MSVC (Visual Studio 2022) /std:c++23preview
// Standard: C++23

#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <stdexcept>
#include <random>
#include <numeric>
#include <functional>
#include <compare>

struct TestVariant {
    std::vector<size_t> question_idxs;
    auto operator<=>(const TestVariant&) const = default;
};

class TestGenerator {
private:
    std::vector<std::string> questions;
    size_t questions_in_test;
    std::set<TestVariant> generated_variants;
    std::mt19937 rng;

    void load_questions_from_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Couldn't open file: " + filename);
        }
        std::string line;
        std::string cur_question;
        while (std::getline(file, line)) {
            if (line.find_first_not_of(" \t\n\r") == std::string::npos) {
                if (!cur_question.empty()) {
                    questions.push_back(cur_question);
                    cur_question.clear();
                }
            }
            else {
                if (!cur_question.empty()) {
                    cur_question += " ";
                }
                cur_question += line;
            }
        }
        if (!cur_question.empty()) {
            questions.push_back(cur_question);
        }
        if (questions.empty()) {
            throw std::runtime_error("File is empty or is incorrect.");
        }
    }
public:
    TestGenerator(const std::string& filename, size_t k)
        : questions_in_test(k), rng(std::random_device{}()) {
        load_questions_from_file(filename);
        if (questions_in_test == 0 || questions_in_test > questions.size()) {
            throw std::invalid_argument("Incorrect number of questions per test.");
        }
    }
    std::string operator()() {
        size_t n = questions.size();
        std::vector<size_t> all_idxs(n);
        std::iota(all_idxs.begin(), all_idxs.end(), 0);
        bool found_new = false;
        TestVariant current_variant;
        size_t attempts = 0;
        const size_t max_attempts = 1000;
        while (attempts < max_attempts) {
            std::shuffle(all_idxs.begin(), all_idxs.end(), rng);
            std::vector<size_t> selected(all_idxs.begin(), all_idxs.begin() + questions_in_test);
            std::sort(selected.begin(), selected.end());
            TestVariant candidate{ selected };
            if (!generated_variants.contains(candidate)) {
                current_variant = candidate;
                found_new = true;
                break;
            }
            attempts++;
        }
        if (!found_new) {
            std::vector<bool> indicator(n, false);
            std::fill(indicator.begin(), indicator.begin() + questions_in_test, true);
            do {
                std::vector<size_t> comb;
                for (size_t i = 0; i < n; ++i) {
                    if (indicator[i]) comb.push_back(i);
                }
                TestVariant candidate{ comb };
                if (!generated_variants.contains(candidate)) {
                    current_variant = candidate;
                    found_new = true;
                    break;
                }
            } while (std::prev_permutation(indicator.begin(), indicator.end()));
        }
        if (!found_new) {
            throw std::overflow_error("All possible unique test variants have been generated");
        }
        generated_variants.insert(current_variant);
        auto format_test = [this](const TestVariant& variant) -> std::string {
            std::string result = "--- VARIANT ---\n";
            for (size_t idx : variant.question_idxs) {
                result += "- " + questions[idx] + "\n";
            }
            return result;
            };
        return format_test(current_variant);
    }
    size_t get_total_questions_count() const { return questions.size(); }
};

void create_test_file(const std::string& filename, size_t n) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Couldn't open test file");
    }
    for (size_t i = 1; i <= n; ++i) {
        file << "Question #" << i << "\n\n";
    }
}

void run_experiment(size_t n, size_t k, const std::string& filename = "temp_questions.txt") {
    std::function<void(const std::string&)> log_info = [](const std::string& msg) {
        std::cout << "[EXPERIMENT]: " << msg << "\n";
        };
    log_info("Start for N = " + std::to_string(n) + ", K = " + std::to_string(k));
    create_test_file(filename, n);
    size_t count = 0;
    try {
        TestGenerator generator(filename, k);
        while (true) {
            std::string test = generator();
            count++;
        }
    }
    catch (const std::overflow_error& e) {
        std::cout << "Experiment completed.\n";
        std::cout << "Total unique variants created: " << count;
        if (std::equal_to<size_t>{}(n, k)) {
            std::cout << " (since N == K)\n";
        }
        else {
            std::cout << "\nAll possible unique combinations generated.\n";
        }
        std::cout << "Stop reason: " << e.what() << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Unique Test Variant Generator (C++23)\n";
    try {
        size_t n = 0, k = 0;
        if (argc >= 3) {
            n = std::stoull(argv[1]);
            k = std::stoull(argv[2]);
            run_experiment(n, k);
        }
        else {
            std::cout << "Demo-run using external file\n";
            TestGenerator demo_gen("questions_demo.txt", 3);
            std::cout << "Loaded " << demo_gen.get_total_questions_count()
                << " questions from questions_demo.txt\n";
            std::cout << "Sample generated variant:\n";
            std::cout << demo_gen() << "\n";
            std::cout << "Automatic tests:\n";
            run_experiment(4, 2);
            run_experiment(5, 3);
            run_experiment(6, 3);

            std::cout << "Enter N and K:\n";
            std::cout << "N (total questions): ";
            if (std::cin >> n) {
                std::cout << "K (questions per test): ";
                if (std::cin >> k) {
                    run_experiment(n, k);
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}