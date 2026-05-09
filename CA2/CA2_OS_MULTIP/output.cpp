#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

// ساختار داده برای نگهداری نتیجه محاسبات
struct ComputeResult {
    string title;
    double score;
};

// خواندن نتایج از پردازه‌های محاسبه‌گر
vector<ComputeResult> readComputeResults(int pipe_fd, int num_compute_processes) {
    vector<ComputeResult> all_results;
    
    cout << "Output process started, expecting results from " << num_compute_processes << " compute processes" << endl;
    
    for (int i = 0; i < num_compute_processes; i++) {
        // خواندن شناسه پردازه
        int process_id;
        if (read(pipe_fd, &process_id, sizeof(int)) <= 0) {
            cerr << "Error reading process ID from compute process " << i << endl;
            continue;
        }
        
        cout << "Reading results from compute process " << process_id << endl;
        
        // خواندن تعداد نتایج
        int num_results;
        if (read(pipe_fd, &num_results, sizeof(int)) <= 0) {
            cerr << "Error reading number of results from compute process " << process_id << endl;
            continue;
        }
        
        cout << "Expecting " << num_results << " results from compute process " << process_id << endl;
        
        // خواندن نتایج
        for (int j = 0; j < num_results; j++) {
            ComputeResult result;
            
            // خواندن طول عنوان
            int title_length;
            if (read(pipe_fd, &title_length, sizeof(int)) <= 0) {
                cerr << "Error reading title length for result " << j << " from process " << process_id << endl;
                break;
            }
            
            // خواندن عنوان
            char* title_buffer = new char[title_length + 1];
            if (read(pipe_fd, title_buffer, title_length) <= 0) {
                cerr << "Error reading title for result " << j << " from process " << process_id << endl;
                delete[] title_buffer;
                break;
            }
            title_buffer[title_length] = '\0';
            result.title = string(title_buffer);
            delete[] title_buffer;
            
            // خواندن امتیاز
            if (read(pipe_fd, &result.score, sizeof(double)) <= 0) {
                cerr << "Error reading score for " << result.title << " from process " << process_id << endl;
                break;
            }
            
            all_results.push_back(result);
            cout << "Read result: " << result.title << " with score " << result.score << endl;
        }
        
        cout << "Completed reading " << num_results << " results from compute process " << process_id << endl;
    }
    
    cout << "Total results collected: " << all_results.size() << endl;
    
    return all_results;
}

// مرتب‌سازی بازی‌ها بر اساس امتیاز
void sortGamesByScore(vector<ComputeResult>& results) {
    sort(results.begin(), results.end(), [](const ComputeResult& a, const ComputeResult& b) {
        return a.score > b.score; // مرتب‌سازی نزولی بر اساس امتیاز
    });
    
    cout << "Results sorted by score" << endl;
}

// ذخیره نتایج در فایل خروجی
void saveRankingToFile(const vector<ComputeResult>& results, const string& filename) {
    ofstream outFile(filename);
    
    if (!outFile.is_open()) {
        cerr << "Error opening output file: " << filename << endl;
        return;
    }
    
    for (size_t i = 0; i < results.size(); i++) {
        outFile << "Title: " << results[i].title << endl;
        outFile << "Score: " << fixed << setprecision(6) << results[i].score << endl;
        
        if (i < results.size() - 1) {
            outFile << endl;
        }
    }
    
    outFile.close();
    cout << "Ranking saved to " << filename << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_pipe_fd> <num_compute_processes>" << endl;
        return 1;
    }
    
    int input_pipe_fd = atoi(argv[1]);
    int num_compute_processes = atoi(argv[2]);
    
    cout << "Output process started" << endl;
    
    // خواندن نتایج از پردازه‌های محاسبه‌گر
    vector<ComputeResult> results = readComputeResults(input_pipe_fd, num_compute_processes);
    
    // بستن لوله ورودی
    close(input_pipe_fd);
    
    if (results.empty()) {
        cerr << "No results were received from compute processes" << endl;
        return 1;
    }
    
    // مرتب‌سازی بازی‌ها بر اساس امتیاز
    sortGamesByScore(results);
    
    // ذخیره نتایج در فایل خروجی
    saveRankingToFile(results, "GameRanking.csv");
    
    cout << "Output process completed" << endl;
    
    return 0;
}