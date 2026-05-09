#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

using namespace std;

// ساختار داده برای نگهداری اطلاعات هر بازی
struct GameData {
    string title;
    double originalPrice;
    double discountPercent;
    int recentSummary;
    int allSummary;
    int recentNum;
    int allNum;
};

// ساختار داده برای نگهداری کمینه و بیشینه هر ستون
struct MinMaxData {
    double minOriginalPrice, maxOriginalPrice;
    double minDiscountPercent, maxDiscountPercent;
    int minRecentSummary, maxRecentSummary;
    int minAllSummary, maxAllSummary;
    int minRecentNum, maxRecentNum;
    int minAllNum, maxAllNum;
};

// خواندن داده‌های پردازش شده از لوله
vector<GameData> readProcessedData(int pipe_fd) {
    vector<GameData> games;
    
    // خواندن تعداد بازی‌ها
    int num_games;
    if (read(pipe_fd, &num_games, sizeof(int)) <= 0) {
        cerr << "Error reading number of games" << endl;
        return games; // لوله بسته شده یا خطا رخ داده است
    }
    
    cout << "Reading " << num_games << " games from extract_transform process" << endl;
    
    // خواندن اطلاعات هر بازی
    for (int i = 0; i < num_games; i++) {
        GameData game;
        
        // خواندن طول عنوان
        int title_length;
        if (read(pipe_fd, &title_length, sizeof(int)) <= 0) {
            cerr << "Error reading title length for game " << i << endl;
            break;
        }
        
        // خواندن عنوان
        char* title_buffer = new char[title_length + 1];
        if (read(pipe_fd, title_buffer, title_length) <= 0) {
            cerr << "Error reading title for game " << i << endl;
            delete[] title_buffer;
            break;
        }
        title_buffer[title_length] = '\0';
        game.title = string(title_buffer);
        delete[] title_buffer;
        
        // خواندن سایر اطلاعات عددی
        if (read(pipe_fd, &game.originalPrice, sizeof(double)) <= 0 ||
            read(pipe_fd, &game.discountPercent, sizeof(double)) <= 0 ||
            read(pipe_fd, &game.recentSummary, sizeof(int)) <= 0 ||
            read(pipe_fd, &game.allSummary, sizeof(int)) <= 0 ||
            read(pipe_fd, &game.recentNum, sizeof(int)) <= 0 ||
            read(pipe_fd, &game.allNum, sizeof(int)) <= 0) {
            
            cerr << "Error reading data for game " << game.title << endl;
            break;
        }
        
        games.push_back(game);
        cout << "Read game: " << game.title << endl;
    }
    
    cout << "Successfully read " << games.size() << " games" << endl;
    
    return games;
}

// محاسبه کمینه و بیشینه هر ستون
MinMaxData calculateMinMax(const vector<GameData>& games) {
    MinMaxData minmax;
    
    if (games.empty()) {
        // مقداردهی پیش‌فرض در صورت خالی بودن لیست
        minmax.minOriginalPrice = minmax.maxOriginalPrice = 0;
        minmax.minDiscountPercent = minmax.maxDiscountPercent = 0;
        minmax.minRecentSummary = minmax.maxRecentSummary = 0;
        minmax.minAllSummary = minmax.maxAllSummary = 0;
        minmax.minRecentNum = minmax.maxRecentNum = 0;
        minmax.minAllNum = minmax.maxAllNum = 0;
        return minmax;
    }
    
    // مقداردهی اولیه با مقادیر بازی اول
    minmax.minOriginalPrice = minmax.maxOriginalPrice = games[0].originalPrice;
    minmax.minDiscountPercent = minmax.maxDiscountPercent = games[0].discountPercent;
    minmax.minRecentSummary = minmax.maxRecentSummary = games[0].recentSummary;
    minmax.minAllSummary = minmax.maxAllSummary = games[0].allSummary;
    minmax.minRecentNum = minmax.maxRecentNum = games[0].recentNum;
    minmax.minAllNum = minmax.maxAllNum = games[0].allNum;
    
    // بررسی سایر بازی‌ها
    for (size_t i = 1; i < games.size(); i++) {
        minmax.minOriginalPrice = min(minmax.minOriginalPrice, games[i].originalPrice);
        minmax.maxOriginalPrice = max(minmax.maxOriginalPrice, games[i].originalPrice);
        
        minmax.minDiscountPercent = min(minmax.minDiscountPercent, games[i].discountPercent);
        minmax.maxDiscountPercent = max(minmax.maxDiscountPercent, games[i].discountPercent);
        
        minmax.minRecentSummary = min(minmax.minRecentSummary, games[i].recentSummary);
        minmax.maxRecentSummary = max(minmax.maxRecentSummary, games[i].recentSummary);
        
        minmax.minAllSummary = min(minmax.minAllSummary, games[i].allSummary);
        minmax.maxAllSummary = max(minmax.maxAllSummary, games[i].allSummary);
        
        minmax.minRecentNum = min(minmax.minRecentNum, games[i].recentNum);
        minmax.maxRecentNum = max(minmax.maxRecentNum, games[i].recentNum);
        
        minmax.minAllNum = min(minmax.minAllNum, games[i].allNum);
        minmax.maxAllNum = max(minmax.maxAllNum, games[i].allNum);
    }
    
    cout << "Min/Max values calculated:" << endl;
    cout << "Original Price: " << minmax.minOriginalPrice << " to " << minmax.maxOriginalPrice << endl;
    cout << "Discount Percent: " << minmax.minDiscountPercent << " to " << minmax.maxDiscountPercent << endl;
    cout << "Recent Summary: " << minmax.minRecentSummary << " to " << minmax.maxRecentSummary << endl;
    cout << "All Summary: " << minmax.minAllSummary << " to " << minmax.maxAllSummary << endl;
    cout << "Recent Number: " << minmax.minRecentNum << " to " << minmax.maxRecentNum << endl;
    cout << "All Number: " << minmax.minAllNum << " to " << minmax.maxAllNum << endl;
    
    return minmax;
}

// دریافت میزان استفاده از CPU برای هر پردازه محاسبه‌گر
vector<double> getCPUUsage(int num_compute_processes) {
    vector<double> cpu_usage(num_compute_processes);
    
    // در یک پیاده‌سازی واقعی، این بخش باید از سیستم‌کال‌های مناسب استفاده کند
    // برای این مثال، مقادیر تصادفی تولید می‌کنیم
    srand(time(NULL));
    for (int i = 0; i < num_compute_processes; i++) {
        cpu_usage[i] = (double)(rand() % 100) / 100.0;
        cout << "CPU usage for compute process " << i << ": " << cpu_usage[i] * 100 << "%" << endl;
    }
    
    return cpu_usage;
}

// ارسال داده‌ها به پردازه‌های محاسبه‌گر با توجه به بار CPU
void distributeDataToComputeNodes(const vector<GameData>& games, 
                                 const MinMaxData& minmax,
                                 int num_compute_processes,
                                 const vector<int>& compute_pipe_fds) {
    if (games.empty() || num_compute_processes <= 0) {
        cerr << "No games to distribute or no compute processes" << endl;
        return;
    }
    
    cout << "Distributing " << games.size() << " games to " << num_compute_processes << " compute processes" << endl;
    
    // دریافت میزان استفاده از CPU برای هر پردازه محاسبه‌گر
    vector<double> cpu_usage = getCPUUsage(num_compute_processes);
    
    // ایجاد لیست اندیس‌های مرتب شده بر اساس میزان استفاده از CPU (کمترین به بیشترین)
    vector<int> sorted_indices(num_compute_processes);
    for (int i = 0; i < num_compute_processes; i++) {
        sorted_indices[i] = i;
    }
    
    sort(sorted_indices.begin(), sorted_indices.end(),
        [&cpu_usage](int a, int b) {
            return cpu_usage[a] < cpu_usage[b];
        });
    
    cout << "Processes sorted by CPU usage: ";
    for (int idx : sorted_indices) {
        cout << idx << " ";
    }
    cout << endl;
    
    // تقسیم بازی‌ها بین پردازه‌های محاسبه‌گر
    int games_per_process = games.size() / num_compute_processes;
    int remaining_games = games.size() % num_compute_processes;
    
    int start_index = 0;
    
    for (int i = 0; i < num_compute_processes; i++) {
        int target_process = sorted_indices[i];
        int pipe_fd = compute_pipe_fds[target_process];
        
        // تعیین تعداد بازی‌ها برای این پردازه
        int num_games_for_process = games_per_process;
        if (i < remaining_games) {
            num_games_for_process++;
        }
        
        cout << "Sending " << num_games_for_process << " games to compute process " << target_process << endl;
        
        // ارسال کمینه و بیشینه به پردازه محاسبه‌گر
        if (write(pipe_fd, &minmax, sizeof(MinMaxData)) != sizeof(MinMaxData)) {
            cerr << "Error writing min-max data to pipe for process " << target_process << endl;
            continue;
        }
        
        // ارسال تعداد بازی‌ها
        if (write(pipe_fd, &num_games_for_process, sizeof(int)) != sizeof(int)) {
            cerr << "Error writing number of games to pipe for process " << target_process << endl;
            continue;
        }
        
        // ارسال اطلاعات هر بازی
        for (int j = 0; j < num_games_for_process; j++) {
            const GameData& game = games[start_index + j];
            
            // ارسال طول عنوان
            int title_length = game.title.length();
            if (write(pipe_fd, &title_length, sizeof(int)) != sizeof(int)) {
                cerr << "Error writing title length to pipe for game " << game.title << endl;
                continue;
            }
            
            // ارسال عنوان
            if (write(pipe_fd, game.title.c_str(), title_length) != title_length) {
                cerr << "Error writing title to pipe for game " << game.title << endl;
                continue;
            }
            
            // ارسال سایر اطلاعات عددی
            if (write(pipe_fd, &game.originalPrice, sizeof(double)) != sizeof(double)) {
                cerr << "Error writing original price to pipe for game " << game.title << endl;
                continue;
            }
            if (write(pipe_fd, &game.discountPercent, sizeof(double)) != sizeof(double)) {
                cerr << "Error writing discount percent to pipe for game " << game.title << endl;
                continue;
            }
            if (write(pipe_fd, &game.recentSummary, sizeof(int)) != sizeof(int)) {
                cerr << "Error writing recent summary to pipe for game " << game.title << endl;
                continue;
            }
            if (write(pipe_fd, &game.allSummary, sizeof(int)) != sizeof(int)) {
                cerr << "Error writing all summary to pipe for game " << game.title << endl;
                continue;
            }
            if (write(pipe_fd, &game.recentNum, sizeof(int)) != sizeof(int)) {
                cerr << "Error writing recent number to pipe for game " << game.title << endl;
                continue;
            }
            if (write(pipe_fd, &game.allNum, sizeof(int)) != sizeof(int)) {
                cerr << "Error writing all number to pipe for game " << game.title << endl;
                continue;
            }
            
            cout << "Sent game: " << game.title << " to compute process " << target_process << endl;
        }
        
        start_index += num_games_for_process;
    }
    
    cout << "All games distributed to compute processes" << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " <num_extract_processes> <num_compute_processes> <extract_pipe_fd1> [extract_pipe_fd2] ... <compute_pipe_fd1> [compute_pipe_fd2] ..." << endl;
        return 1;
    }
    
    int num_extract_processes = atoi(argv[1]);
    int num_compute_processes = atoi(argv[2]);
    
    if (argc != 3 + num_extract_processes + num_compute_processes) {
        cerr << "Incorrect number of arguments" << endl;
        return 1;
    }
    
    cout << "Load process started with " << num_extract_processes << " extract processes and " 
         << num_compute_processes << " compute processes" << endl;
    
    // تبدیل آرگومان‌ها به توصیف‌گر فایل‌های لوله
    vector<int> extract_pipe_fds(num_extract_processes);
    for (int i = 0; i < num_extract_processes; i++) {
        extract_pipe_fds[i] = atoi(argv[3 + i]);
    }
    
    vector<int> compute_pipe_fds(num_compute_processes);
    for (int i = 0; i < num_compute_processes; i++) {
        compute_pipe_fds[i] = atoi(argv[3 + num_extract_processes + i]);
    }
    
    // خواندن داده‌ها از پردازه‌های استخراج و تبدیل
    vector<GameData> all_games;
    for (int i = 0; i < num_extract_processes; i++) {
        cout << "Reading data from extract process " << i << endl;
        vector<GameData> games = readProcessedData(extract_pipe_fds[i]);
        all_games.insert(all_games.end(), games.begin(), games.end());
        close(extract_pipe_fds[i]);
    }
    
    if (all_games.empty()) {
        cerr << "No games were received from extract processes" << endl;
        return 1;
    }
    
    cout << "Total games received: " << all_games.size() << endl;
    
    // محاسبه کمینه و بیشینه هر ستون
    MinMaxData minmax = calculateMinMax(all_games);
    
    // توزیع داده‌ها به پردازه‌های محاسبه‌گر
    distributeDataToComputeNodes(all_games, minmax, num_compute_processes, compute_pipe_fds);
    
    // بستن لوله‌ها
    for (int pipe_fd : compute_pipe_fds) {
        close(pipe_fd);
    }
    
    cout << "Load process completed" << endl;
    
    return 0;
}