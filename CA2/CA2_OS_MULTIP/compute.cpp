#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <fcntl.h>

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
    double score;
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

// ساختار داده برای نگهداری نتیجه محاسبات
struct ComputeResult {
    string title;
    double score;
};

double minMaxScaling(double value, double min, double max) {
    if (max == min) return 1.0; // برای جلوگیری از تقسیم بر صفر
    return ((value - min) / (max - min)) + 1.0;
}

// محاسبه امتیاز محبوبیت بازی
vector<ComputeResult> calculateScores(const vector<GameData>& games, const MinMaxData& minmax) {
    vector<ComputeResult> results;
    
    for (const auto& game : games) {
        ComputeResult result;
        result.title = game.title;
        
        // مقیاس‌بندی داده‌ها
        double scaledOriginalPrice = minMaxScaling(game.originalPrice, minmax.minOriginalPrice, minmax.maxOriginalPrice);
        double scaledDiscountPercent = minMaxScaling(game.discountPercent, minmax.minDiscountPercent, minmax.maxDiscountPercent);
        double scaledRecentSummary = minMaxScaling(game.recentSummary, minmax.minRecentSummary, minmax.maxRecentSummary);
        double scaledAllSummary = minMaxScaling(game.allSummary, minmax.minAllSummary, minmax.maxAllSummary);
        double scaledRecentNum = minMaxScaling(game.recentNum, minmax.minRecentNum, minmax.maxRecentNum);
        double scaledAllNum = minMaxScaling(game.allNum, minmax.minAllNum, minmax.maxAllNum);
        
        result.score = (10 * scaledDiscountPercent * scaledRecentSummary * scaledAllSummary * 
              scaledRecentNum * scaledAllNum) / scaledOriginalPrice;
        
        results.push_back(result);
        cout << "Calculated score for " << result.title << ": " << result.score << endl;
    }
    
    return results;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <input_pipe_fd> <output_pipe_fd> <process_id>" << endl;
        return 1;
    }
    
    int input_pipe_fd = atoi(argv[1]);
    int output_pipe_fd = atoi(argv[2]);
    int process_id = atoi(argv[3]);
    
    cout << "Compute process " << process_id << " started" << endl;
    
    // خواندن کمینه و بیشینه هر ستون
    MinMaxData minmax;
    if (read(input_pipe_fd, &minmax, sizeof(MinMaxData)) <= 0) {
        cerr << "Error reading min-max data" << endl;
        return 1;
    }
    
    cout << "Min-max data received by compute process " << process_id << endl;
    
    // خواندن تعداد بازی‌ها
    int num_games;
    if (read(input_pipe_fd, &num_games, sizeof(int)) <= 0) {
        cerr << "Error reading number of games" << endl;
        return 1;
    }
    
    cout << "Compute process " << process_id << " received " << num_games << " games" << endl;
    
    // خواندن اطلاعات بازی‌ها
    vector<GameData> games;
    for (int i = 0; i < num_games; i++) {
        GameData game;
        
        // خواندن طول عنوان
        int title_length;
        if (read(input_pipe_fd, &title_length, sizeof(int)) <= 0) {
            cerr << "Error reading title length for game " << i << endl;
            break;
        }
        
        // خواندن عنوان
        char* title_buffer = new char[title_length + 1];
        if (read(input_pipe_fd, title_buffer, title_length) <= 0) {
            cerr << "Error reading title for game " << i << endl;
            delete[] title_buffer;
            break;
        }
        title_buffer[title_length] = '\0';
        game.title = string(title_buffer);
        delete[] title_buffer;
        
        // خواندن سایر اطلاعات عددی
        if (read(input_pipe_fd, &game.originalPrice, sizeof(double)) <= 0 ||
            read(input_pipe_fd, &game.discountPercent, sizeof(double)) <= 0 ||
            read(input_pipe_fd, &game.recentSummary, sizeof(int)) <= 0 ||
            read(input_pipe_fd, &game.allSummary, sizeof(int)) <= 0 ||
            read(input_pipe_fd, &game.recentNum, sizeof(int)) <= 0 ||
            read(input_pipe_fd, &game.allNum, sizeof(int)) <= 0) {
            
            cerr << "Error reading data for game " << game.title << endl;
            break;
        }
        
        games.push_back(game);
        cout << "Compute process " << process_id << " received game: " << game.title << endl;
    }
    
    // بستن لوله ورودی
    close(input_pipe_fd);
    
    cout << "Compute process " << process_id << " calculating scores for " << games.size() << " games" << endl;
    
    // محاسبه امتیازها
    vector<ComputeResult> results = calculateScores(games, minmax);
    
    cout << "Compute process " << process_id << " sending results to output process" << endl;
    
    // ارسال شناسه پردازه
    if (write(output_pipe_fd, &process_id, sizeof(int)) != sizeof(int)) {
        cerr << "Error writing process ID to output pipe" << endl;
        return 1;
    }
    
    // ارسال تعداد نتایج
    int num_results = results.size();
    if (write(output_pipe_fd, &num_results, sizeof(int)) != sizeof(int)) {
        cerr << "Error writing number of results to output pipe" << endl;
        return 1;
    }
    
    // ارسال نتایج
    for (const auto& result : results) {
        // ارسال طول عنوان
        int title_length = result.title.length();
        if (write(output_pipe_fd, &title_length, sizeof(int)) != sizeof(int)) {
            cerr << "Error writing title length to output pipe for " << result.title << endl;
            continue;
        }
        
        // ارسال عنوان
        if (write(output_pipe_fd, result.title.c_str(), title_length) != title_length) {
            cerr << "Error writing title to output pipe for " << result.title << endl;
            continue;
        }
        
        // ارسال امتیاز
        if (write(output_pipe_fd, &result.score, sizeof(double)) != sizeof(double)) {
            cerr << "Error writing score to output pipe for " << result.title << endl;
            continue;
        }
        
        cout << "Compute process " << process_id << " sent result for " << result.title << endl;
    }
    
    // بستن لوله خروجی
    close(output_pipe_fd);
    
    cout << "Compute process " << process_id << " completed" << endl;
    
    return 0;
}