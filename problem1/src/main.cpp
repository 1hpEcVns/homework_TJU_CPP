#include <print>       // C++23 printing
#include <vector>      // For storing student data and processing steps
#include <expected>    // C++23 for error handling
#include <string>      // For error messages and string views
#include <string_view> // For passing titles efficiently
#include <span>        // C++20 for non-owning views of data
#include <numeric>     // For std::accumulate
#include <random>      // For data generation
#include <chrono>      // For seeding random generator
#include <algorithm>   // For std::ranges::sort, std::clamp
#include <functional>  // For std::function
#include <format>      // For explicit formatting if needed
#include <ranges>      // For views and range algorithms
#include <cstddef>     // For size_t
#include <thread>      // For sleep
#include <utility>     // For std::move
#include <concepts>    // For defining range concepts
#include <iterator>    // For std::begin, std::end if needed with std::accumulate

// --- Structs, Concepts, Constants  ---
struct Student {
    int id;
    double score;
};
template <typename R>
concept StudentRange = std::ranges::input_range<R> &&
                      std::same_as<std::ranges::range_value_t<R>, Student>;

constexpr size_t NUM_STUDENTS = 30;
constexpr double MAX_SCORE = 100.0;
constexpr double MIN_SCORE = 0.0;
constexpr double PASS_THRESHOLD = 60.0;
constexpr double EXCELLENT_THRESHOLD = 85.0;
constexpr double SCORE_MEAN_CENTER = 70.0;
constexpr double SCORE_STD_DEV = 30.0;

using SingleStudentResult = std::expected<Student, std::string>;

// --- generate_single_student  ---
SingleStudentResult generate_single_student(int student_id) {
    static std::mt19937 engine(std::chrono::system_clock::now().time_since_epoch().count());
    std::normal_distribution<double> score_dist(SCORE_MEAN_CENTER, SCORE_STD_DEV);
    std::uniform_int_distribution<int> error_injector(1, 20);
    double generated_score = score_dist(engine);
    bool injected_error = (error_injector(engine) == 1);

    if (injected_error) {
        return std::unexpected(std::format("Generation failed: {}", "Simulated random error"));
    }
    if (generated_score < MIN_SCORE || generated_score > MAX_SCORE) {
         return std::unexpected(std::format(
            "Generation failed: Raw score {:.2f} out of range [{:.1f}, {:.1f}]",
            generated_score, MIN_SCORE, MAX_SCORE));
    }
    return Student{student_id, generated_score};
}


// --- print_student_table  ---
void print_student_table(
    std::string_view list_title,
    StudentRange auto&& student_range, // Accept any range of Students
    bool print_summary_count)
{
    constexpr int W_ID = 10;
    constexpr int W_SCORE = 12;
    const size_t TABLE_WIDTH = W_ID + W_SCORE + 7;

    std::println("--- {} ---", list_title); // Sub-header for the list
    std::println("| {:<{}} | {:<{}} |", "Student ID", W_ID, "Score", W_SCORE);
    std::println("|{}|{}|", std::string(W_ID + 2, '-'), std::string(W_SCORE + 2, '-'));

    size_t count = 0;

    for (const auto& student : student_range) {
        std::println("| {:<{}} | {:<{}.2f} |", student.id, W_ID, student.score, W_SCORE);
        count++;
    }


    std::println("{}", std::string(TABLE_WIDTH, '-'));
    if (print_summary_count) {
        std::println("Total matching students: {}", count);
        if (count == 0) {
             std::println("(No students met the criteria for this list)");
        }
    }
    std::println("");
}


// --- ProcessingStep struct  ---
struct ProcessingStep {
    std::string main_title;
    std::function<void(std::vector<Student>&)> core_logic; // Can operate on mutable data
};

// --- execute_processing_step  ---
void execute_processing_step(
    const ProcessingStep& step,
    std::vector<Student>& data) // Pass mutable data
{
    std::println("\n========== {} ==========", step.main_title);
    if (data.empty() && step.main_title != "(Hypothetical Static Step)") {
        std::println("--- No student data available to process for this step ---");
        std::println("");
        return;
    }
    // Execute the core logic, which now expects a mutable reference
    step.core_logic(data);
}

// --- Factory Functions  ---

// Filter & Print (Operates on const data indirectly via core_logic wrapper)
template <typename FilterPredicate>
ProcessingStep make_filter_print_step(
    std::string main_title,
    std::string list_title,
    FilterPredicate filter,
    bool print_summary)
{
    return {
        .main_title = std::move(main_title),
        .core_logic = [=, filter = std::move(filter),
                       list_title = std::move(list_title)]
                      (std::vector<Student>& data) 
                      {
                          std::span<const Student> view = data;
                          print_student_table(list_title, view | std::views::filter(filter), print_summary);
                      }
    };
}

// Action Step (Operates on mutable data)
ProcessingStep make_action_step(
    std::string main_title,
    std::function<void(std::vector<Student>&)> action // Expects mutable ref
    )
{
    return {
        .main_title = std::move(main_title),
        .core_logic = std::move(action) // Directly use the provided action
    };
}

// Custom Logic Step (Operates on const data indirectly via core_logic wrapper)
ProcessingStep make_custom_logic_step(
    std::string main_title,
    std::function<void(const std::vector<Student>&)> logic // Logic itself takes const
    )
{
     return {
        .main_title = std::move(main_title),
        // Core logic lambda takes mutable ref but passes const ref internally
        .core_logic = [logic = std::move(logic)](std::vector<Student>& data) { // Takes mutable
             const std::vector<Student>& const_data = data; // Pass const
             logic(const_data);
        }
    };
}


// --- Main Program ---
int main() {
    std::vector<Student> students;
    students.reserve(NUM_STUDENTS);
    size_t current_id_index = 0;

    std::println("========== Generating Data for {} Students (Normal Dist., Retry on Error) ==========", NUM_STUDENTS);
    while (students.size() < NUM_STUDENTS) {
        const int target_id = static_cast<int>(current_id_index + 1);
        size_t attempt_count = 0;
        std::print("  Generating data for ID {:<4}...", target_id);
        while (true) {
            attempt_count++;
            SingleStudentResult result = generate_single_student(target_id);
            if (result) {
                students.push_back(result.value());
                std::println(" [OK] Score: {:.2f} (Attempt {})", result.value().score, attempt_count);
                current_id_index++;
                break;
            } else {
                std::print("\n    [!!) Attempt {} Failed: {}. Retrying...", attempt_count, result.error());
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
    }
    std::println("======= Generation Complete: {} Students Generated =======", students.size());


    std::println("\n========== Processing Student Data ==========");

    // --- Define Processing Steps using Vector Constructor and Factory Functions ---
    // Initialize the vector directly using list initialization { }
    const auto processing_steps = std::vector{

        // Step 1: Excellent Students (Filter & Print)
        make_filter_print_step(
            "(1) Filter: Excellent Students",
            std::format("List: Score > {:.1f}", EXCELLENT_THRESHOLD),
            [](const Student& s){ return s.score > EXCELLENT_THRESHOLD; },
            true
        ), // Comma separates elements

        // Step 2: Failing Students (Filter & Print)
        make_filter_print_step(
            "(2) Filter: Failing Students",
            std::format("List: Score < {:.1f}", PASS_THRESHOLD),
            [](const Student& s){ return s.score < PASS_THRESHOLD; },
            true
        ),

        // Step 3: Calculate Average and Print Students Above Average (Custom Logic - FIXED)
        make_custom_logic_step(
            "(3) Calculate & Filter: Above Average",
            [](const std::vector<Student>& data) { // Logic lambda takes const ref
                 if (data.empty()) { // Handle empty data case explicitly here too
                     std::println("--- Statistics ---");
                     std::println("Number of students analyzed: 0");
                     std::println("Calculated Average Score: N/A");
                     std::println("--------------------");
                     print_student_table("List: Scoring >= Average (N/A)", std::span<const Student>{}, true); // Pass empty span
                     return;
                 }

                 std::span<const Student> view = data;

                 // FIX 2: Use std::accumulate from <numeric> with iterators
                 double sum_of_scores = std::accumulate(view.begin(), view.end(), 0.0,
                                                       [](double current_sum, const Student& s) { return current_sum + s.score; });

                 double average_score = sum_of_scores / view.size(); // Avoid division by zero checked above

                 std::println("--- Statistics ---");
                 std::println("Number of students analyzed: {}", view.size());
                 std::println("Calculated Average Score: {:.2f}", average_score);
                 std::println("--------------------");

                 print_student_table(
                     std::format("List: Scoring >= Average ({:.2f})", average_score),
                     view | std::views::filter([average_score](const Student& s){ return s.score >= average_score; })
                     , 
                     true
                 );
            }
        ),

        // Step 4: Sort and Print All
        make_action_step(
             "(4) Action & View: Sort All and Print",
             [](std::vector<Student>& data_to_sort_and_print) {
                 std::println("--- Sorting Data by Score (Descending)... ---");
                 // Using std::ranges::sort is correct here
                 std::ranges::sort(data_to_sort_and_print, std::greater<>{}, &Student::score);
                 std::println("--- Data Sorted Successfully ---");
                 std::println(""); // Maintain spacing

                 print_student_table(
                     "List: All Students (Sorted by Score Descending)", // List title from original Step 5
                     data_to_sort_and_print, // Pass the now-sorted data
                     false
                 );
             }
         )
    };
    // Execute the steps
    for (const auto& step : processing_steps) {
         execute_processing_step(step, students);
    }

    std::println("\n========== Processing Complete ==========");

    return 0;
}