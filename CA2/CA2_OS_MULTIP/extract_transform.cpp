#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

// ساختار داده برای نگهداری اطلاعات پردازش شده هر بازی
struct ProcessedGame {
    string title;
    double originalPrice;
    double discountPercent;
    int recentSummary;
    int allSummary;
    int recentNum;
    int allNum;
};

// تبدیل خلاصه نظرات به مقدار عددی
int convertSummaryToNumber(const string& summary) {
    if (summary.find("Overwhelmingly Positive") != string::npos) return 7;
    if (summary.find("Very Positive") != string::npos) return 6;
    if (summary.find("Positive") != string::npos && summary.find("Mostly") == string::npos) return 5;
    if (summary.find("Mostly Positive") != string::npos) return 4;
    if (summary.find("Mixed") != string::npos) return 3;
    if (summary.find("Mostly Negative") != string::npos) return 2;
    if (summary.find("Negative") != string::npos && summary.find("Mostly") == string::npos 
        && summary.find("Overwhelmingly") == string::npos) return 1;
    if (summary.find("Overwhelmingly Negative") != string::npos) return 0;
    return 3; // برای مقادیر نامشخص، Mixed در نظر گرفته می‌شود
}

// استخراج تعداد نظرات از رشته
int extractReviewsNumber(const string& reviewsStr) {
    size_t pos = reviewsStr.find("of the");
    if (pos != string::npos) {
        string numStr = reviewsStr.substr(pos + 7);
        pos = numStr.find("user");
        if (pos != string::npos) {
            numStr = numStr.substr(0, pos);
            // حذف کاراکترهای غیر عددی
            numStr.erase(remove_if(numStr.begin(), numStr.end(), 
                [](char c) { return !isdigit(c); }), numStr.end());
            
            if (!numStr.empty()) {
                return stoi(numStr);
            }
        }
    }
    return 0;
}

// حذف کاراکتر $ از قیمت و تبدیل به عدد
double extractPrice(const string& priceStr) {
    string numStr = priceStr;
    numStr.erase(remove_if(numStr.begin(), numStr.end(), 
        [](char c) { return c == '$' || c == ' '; }), numStr.end());
    
    try {
        if (!numStr.empty()) {
            return stod(numStr);
        }
    } catch (...) {
        // در صورت خطا
    }
    return 1.0; // اگر قیمت قابل تبدیل نبود، 1 در نظر گرفته می‌شود
}

// محاسبه درصد تخفیف
double calculateDiscountPercent(double original, double discounted) {
    if (original <= 0 || discounted <= 0) return 1.0;
    double percent = 100.0 - (discounted / original * 100.0);
    return max(1.0, percent);
}

// خواندن و پردازش فایل CSV
vector<ProcessedGame> processCSVFile(const string& filename) {
    vector<ProcessedGame> games;
    ifstream file(filename);
    
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return games;
    }
    
    cout << "Successfully opened file: " << filename << endl;
    
    string line;
    bool isStandardCSV = false;
    
    // بررسی فرمت فایل
    if (getline(file, line)) {
        isStandardCSV = (line.find(',') != string::npos);
    }
    
    // برگشت به ابتدای فایل
    file.clear();
    file.seekg(0);
    
    if (isStandardCSV) {
        // پردازش فایل CSV استاندارد
        cout << "Detected standard CSV format for file: " << filename << endl;
        
        // خواندن سرستون‌ها
        getline(file, line);
        
        // پردازش هر خط
        while (getline(file, line)) {
            stringstream ss(line);
            string title, originalPrice, discountedPrice, recentSummary, allSummary, recentNum, allNum;
            
            // خواندن ستون‌های مورد نیاز
            getline(ss, title, ',');
            getline(ss, originalPrice, ',');
            getline(ss, discountedPrice, ',');
            
            // رد کردن ستون‌های اضافی
            string temp;
            getline(ss, temp, ','); // Link
            getline(ss, temp, ','); // Game Description
            
            getline(ss, recentSummary, ',');
            getline(ss, allSummary, ',');
            getline(ss, recentNum, ',');
            getline(ss, allNum, ',');
            
            ProcessedGame game;
            game.title = title;
            game.originalPrice = extractPrice(originalPrice);
            double discountedPrice_val = extractPrice(discountedPrice);
            game.discountPercent = calculateDiscountPercent(game.originalPrice, discountedPrice_val);
            game.recentSummary = convertSummaryToNumber(recentSummary);
            game.allSummary = convertSummaryToNumber(allSummary);
            game.recentNum = extractReviewsNumber(recentNum);
            game.allNum = extractReviewsNumber(allNum);
            
            games.push_back(game);
            cout << "Parsed game: " << game.title << endl;
        }
    } else {
        // پردازش فایل با فرمت خاص
        cout << "Detected custom format for file: " << filename << endl;
        
        // خواندن خط به خط فایل
        while (getline(file, line)) {
            if (line.find("Title:") != string::npos) {
                ProcessedGame game;
                
                // استخراج عنوان بازی
                game.title = line.substr(line.find("Title:") + 7);
                // فقط کلمه اول عنوان را نگه می‌داریم
                size_t spacePos = game.title.find(' ');
                if (spacePos != string::npos) {
                    game.title = game.title.substr(0, spacePos);
                }
                
                // خواندن قیمت اصلی
                getline(file, line);
                if (line.find("Original Price:") != string::npos) {
                    game.originalPrice = extractPrice(line.substr(line.find("Original Price:") + 15));
                    cout << "Original price: " << game.originalPrice << endl;
                }
                
                // خواندن قیمت با تخفیف
                getline(file, line);
                double discountedPrice = 0.0;
                if (line.find("Discounted Price:") != string::npos) {
                    discountedPrice = extractPrice(line.substr(line.find("Discounted Price:") + 17));
                    cout << "Discounted price: " << discountedPrice << endl;
                }
                
                // محاسبه درصد تخفیف
                game.discountPercent = calculateDiscountPercent(game.originalPrice, discountedPrice);
                cout << "Discount percent: " << game.discountPercent << "%" << endl;
                
                // رد کردن لینک
                getline(file, line); // Link
                
                // رد کردن توضیحات بازی
                getline(file, line); // Game Description
                
                // خواندن خلاصه نظرات اخیر
                getline(file, line);
                if (line.find("Recent Reviews Summary:") != string::npos) {
                    game.recentSummary = convertSummaryToNumber(line.substr(line.find("Recent Reviews Summary:") + 22));
                    cout << "Recent summary: " << game.recentSummary << endl;
                }
                
                // خواندن خلاصه تمام نظرات
                getline(file, line);
                if (line.find("All Reviews Summary:") != string::npos) {
                    game.allSummary = convertSummaryToNumber(line.substr(line.find("All Reviews Summary:") + 20));
                    cout << "All summary: " << game.allSummary << endl;
                }
                
                // خواندن تعداد نظرات اخیر
                getline(file, line);
                if (line.find("Recent Reviews Number:") != string::npos) {
                    game.recentNum = extractReviewsNumber(line.substr(line.find("Recent Reviews Number:") + 22));
                    cout << "Recent num: " << game.recentNum << endl;
                }
                
                // خواندن تعداد کل نظرات
                getline(file, line);
                if (line.find("All Reviews Number:") != string::npos) {
                    game.allNum = extractReviewsNumber(line.substr(line.find("All Reviews Number:") + 19));
                    cout << "All num: " << game.allNum << endl;
                }
                
                // رد کردن سایر فیلدها که نیاز نداریم
                getline(file, line); // Developer
                getline(file, line); // Publisher
                getline(file, line); // Popular Tags
                getline(file, line); // Game Features
                getline(file, line); // Minimum Requirements
                
                games.push_back(game);
                cout << "Added game: " << game.title << endl;
            }
        }
    }
    
    cout << "Found " << games.size() << " games in " << filename << endl;
    
    file.close();
    return games;
}

// ارسال داده‌های پردازش شده به لوله
void sendProcessedData(int pipe_fd, const vector<ProcessedGame>& games) {
    // ارسال تعداد بازی‌ها
    int num_games = games.size();
    if (write(pipe_fd, &num_games, sizeof(int)) != sizeof(int)) {
        cerr << "Error writing number of games to pipe" << endl;
        return;
    }
    
    cout << "Sending " << num_games << " games to load process" << endl;
    
    // ارسال اطلاعات هر بازی
    for (const auto& game : games) {
        // ارسال طول عنوان
        int title_length = game.title.length();
        if (write(pipe_fd, &title_length, sizeof(int)) != sizeof(int)) {
            cerr << "Error writing title length to pipe" << endl;
            continue;
        }
        
        // ارسال عنوان
        if (write(pipe_fd, game.title.c_str(), title_length) != title_length) {
            cerr << "Error writing title to pipe" << endl;
            continue;
        }
        
        // ارسال سایر اطلاعات عددی
        if (write(pipe_fd, &game.originalPrice, sizeof(double)) != sizeof(double)) {
            cerr << "Error writing original price to pipe" << endl;
            continue;
        }
        if (write(pipe_fd, &game.discountPercent, sizeof(double)) != sizeof(double)) {
            cerr << "Error writing discount percent to pipe" << endl;
            continue;
        }
        if (write(pipe_fd, &game.recentSummary, sizeof(int)) != sizeof(int)) {
            cerr << "Error writing recent summary to pipe" << endl;
            continue;
        }
        if (write(pipe_fd, &game.allSummary, sizeof(int)) != sizeof(int)) {
            cerr << "Error writing all summary to pipe" << endl;
            continue;
        }
        if (write(pipe_fd, &game.recentNum, sizeof(int)) != sizeof(int)) {
            cerr << "Error writing recent number to pipe" << endl;
            continue;
        }
        if (write(pipe_fd, &game.allNum, sizeof(int)) != sizeof(int)) {
            cerr << "Error writing all number to pipe" << endl;
            continue;
        }
        
        cout << "Sent game: " << game.title << endl;
    }
    
    cout << "All games sent to load process" << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <file> <pipe_fd>" << endl;
        return 1;
    }
    
    string filename = argv[1];
    int pipe_fd = atoi(argv[2]);
    
    cout << "Extract_transform process started for file: " << filename << endl;
    
    // پردازش فایل ورودی
    vector<ProcessedGame> games = processCSVFile(filename);
    
    if (games.empty()) {
        cerr << "No games were found in the input file: " << filename << endl;
        return 1;
    }
    
    cout << "Found " << games.size() << " games in " << filename << endl;
    
    // ارسال داده‌های پردازش شده به لوله
    sendProcessedData(pipe_fd, games);
    
    // بستن لوله
    close(pipe_fd);
    
    cout << "Extract_transform process completed for file: " << filename << endl;
    
    return 0;
}