#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <iomanip>

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

// تبدیل خلاصه نظرات به مقدار عددی
int convertSummaryToNumber(const string& summary) {
    if (summary.find("Overwhelmingly Positive") != string::npos) return 7;
    if (summary.find("Very Positive") != string::npos) return 6;
    if (summary.find("Positive") != string::npos && summary.find("Mostly") == string::npos) return 5;
    if (summary.find("Mostly Positive") != string::npos) return 4;
    if (summary.find("Mixed") != string::npos) return 3;
    if (summary.find("Mostly Negative") != string::npos) return 2;
    if (summary.find("Negative") != string::npos && summary.find("Mostly") == string::npos) return 1;
    if (summary.find("Overwhelmingly Negative") != string::npos) return 1;
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
vector<GameData> processCSVFile(const string& filename) {
    vector<GameData> games;
    ifstream file(filename);
    
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return games;
    }
    
    cout << "\nProcessing file: " << filename << endl;
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
            
            GameData game;
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
                GameData game;
                
                // استخراج عنوان بازی
                game.title = line.substr(line.find("Title:") + 7);
                
                // خواندن قیمت اصلی
                getline(file, line);
                if (line.find("Original Price:") != string::npos) {
                    game.originalPrice = extractPrice(line.substr(line.find("Original Price:") + 15));
                }
                
                // خواندن قیمت با تخفیف
                getline(file, line);
                double discountedPrice = 0.0;
                if (line.find("Discounted Price:") != string::npos) {
                    discountedPrice = extractPrice(line.substr(line.find("Discounted Price:") + 17));
                }
                
                // محاسبه درصد تخفیف
                game.discountPercent = calculateDiscountPercent(game.originalPrice, discountedPrice);
                
                // رد کردن لینک
                getline(file, line); // Link
                
                // رد کردن توضیحات بازی
                getline(file, line); // Game Description
                
                // خواندن خلاصه نظرات اخیر
                getline(file, line);
                if (line.find("Recent Reviews Summary:") != string::npos) {
                    game.recentSummary = convertSummaryToNumber(line.substr(line.find("Recent Reviews Summary:") + 22));
                }
                
                // خواندن خلاصه تمام نظرات
                getline(file, line);
                if (line.find("All Reviews Summary:") != string::npos) {
                    game.allSummary = convertSummaryToNumber(line.substr(line.find("All Reviews Summary:") + 20));
                }
                
                // خواندن تعداد نظرات اخیر
                getline(file, line);
                if (line.find("Recent Reviews Number:") != string::npos) {
                    game.recentNum = extractReviewsNumber(line.substr(line.find("Recent Reviews Number:") + 22));
                }
                
                // خواندن تعداد کل نظرات
                getline(file, line);
                if (line.find("All Reviews Number:") != string::npos) {
                    game.allNum = extractReviewsNumber(line.substr(line.find("All Reviews Number:") + 19));
                }
                
                // رد کردن سایر فیلدها که نیاز نداریم
                getline(file, line); // Developer
                getline(file, line); // Publisher
                getline(file, line); // Popular Tags
                getline(file, line); // Game Features
                getline(file, line); // Minimum Requirements
                
                games.push_back(game);
                cout << "Parsed game: " << game.title << endl;
            }
        }
    }
    
    cout << "Read " << (isStandardCSV ? to_string(games.size() + 1) : "?") << " lines from file: " << filename << endl;
    cout << "Found " << games.size() << " games in file: " << filename << endl;
    
    file.close();
    return games;
}

// محاسبه کمینه و بیشینه هر ستون
void calculateMinMax(const vector<GameData>& games, 
                    double& minOriginalPrice, double& maxOriginalPrice,
                    double& minDiscountPercent, double& maxDiscountPercent,
                    int& minRecentSummary, int& maxRecentSummary,
                    int& minAllSummary, int& maxAllSummary,
                    int& minRecentNum, int& maxRecentNum,
                    int& minAllNum, int& maxAllNum) {
    
    if (games.empty()) return;
    
    // مقداردهی اولیه با مقادیر بازی اول
    minOriginalPrice = maxOriginalPrice = games[0].originalPrice;
    minDiscountPercent = maxDiscountPercent = games[0].discountPercent;
    minRecentSummary = maxRecentSummary = games[0].recentSummary;
    minAllSummary = maxAllSummary = games[0].allSummary;
    minRecentNum = maxRecentNum = games[0].recentNum;
    minAllNum = maxAllNum = games[0].allNum;
    
    // بررسی سایر بازی‌ها
    for (size_t i = 1; i < games.size(); i++) {
        minOriginalPrice = min(minOriginalPrice, games[i].originalPrice);
        maxOriginalPrice = max(maxOriginalPrice, games[i].originalPrice);
        
        minDiscountPercent = min(minDiscountPercent, games[i].discountPercent);
        maxDiscountPercent = max(maxDiscountPercent, games[i].discountPercent);
        
        minRecentSummary = min(minRecentSummary, games[i].recentSummary);
        maxRecentSummary = max(maxRecentSummary, games[i].recentSummary);
        
        minAllSummary = min(minAllSummary, games[i].allSummary);
        maxAllSummary = max(maxAllSummary, games[i].allSummary);
        
        minRecentNum = min(minRecentNum, games[i].recentNum);
        maxRecentNum = max(maxRecentNum, games[i].recentNum);
        
        minAllNum = min(minAllNum, games[i].allNum);
        maxAllNum = max(maxAllNum, games[i].allNum);
    }
}

// مقیاس‌بندی داده با روش Min-Max Scaling
double minMaxScaling(double value, double min, double max) {
    if (max == min) return 1.0; // برای جلوگیری از تقسیم بر صفر
    return ((value - min) / (max - min)) + 1.0;
}

// محاسبه امتیاز محبوبیت بازی
void calculateScores(vector<GameData>& games) {
    double minOriginalPrice, maxOriginalPrice;
    double minDiscountPercent, maxDiscountPercent;
    int minRecentSummary, maxRecentSummary;
    int minAllSummary, maxAllSummary;
    int minRecentNum, maxRecentNum;
    int minAllNum, maxAllNum;
    
    calculateMinMax(games, minOriginalPrice, maxOriginalPrice, minDiscountPercent, maxDiscountPercent,
                   minRecentSummary, maxRecentSummary, minAllSummary, maxAllSummary,
                   minRecentNum, maxRecentNum, minAllNum, maxAllNum);
    
    cout << "\nTotal games found across all files: " << games.size() << endl;
    cout << "Min/Max values calculated:" << endl;
    cout << "Original Price: " << minOriginalPrice << " to " << maxOriginalPrice << endl;
    cout << "Discount Percent: " << minDiscountPercent << " to " << maxDiscountPercent << endl;
    cout << "Recent Summary: " << minRecentSummary << " to " << maxRecentSummary << endl;
    cout << "All Summary: " << minAllSummary << " to " << maxAllSummary << endl;
    cout << "Recent Number: " << minRecentNum << " to " << maxRecentNum << endl;
    cout << "All Number: " << minAllNum << " to " << maxAllNum << endl;
    
    for (auto& game : games) {
        // مقیاس‌بندی داده‌ها
        double scaledOriginalPrice = minMaxScaling(game.originalPrice, minOriginalPrice, maxOriginalPrice);
        double scaledDiscountPercent = minMaxScaling(game.discountPercent, minDiscountPercent, maxDiscountPercent);
        double scaledRecentSummary = minMaxScaling(game.recentSummary, minRecentSummary, maxRecentSummary);
        double scaledAllSummary = minMaxScaling(game.allSummary, minAllSummary, maxAllSummary);
        double scaledRecentNum = minMaxScaling(game.recentNum, minRecentNum, maxRecentNum);
        double scaledAllNum = minMaxScaling(game.allNum, minAllNum, maxAllNum);
        
        // محاسبه امتیاز با فرمول داده شده
        game.score = (10 * scaledDiscountPercent * scaledRecentSummary * scaledAllSummary * 
                      scaledRecentNum * scaledAllNum) / scaledOriginalPrice;
        
        cout << "Game: " << game.title << " - Score: " << game.score << endl;
    }
}

// مرتب‌سازی بازی‌ها بر اساس امتیاز
void sortGamesByScore(vector<GameData>& games) {
    sort(games.begin(), games.end(), [](const GameData& a, const GameData& b) {
        return a.score > b.score; // مرتب‌سازی نزولی بر اساس امتیاز
    });
    cout << "Games sorted by score." << endl;
}

// ذخیره نتایج در فایل خروجی
void saveRankingToFile(const vector<GameData>& games, const string& filename) {
    ofstream outFile(filename);
    
    if (!outFile.is_open()) {
        cerr << "Error opening output file: " << filename << endl;
        return;
    }
    
    for (size_t i = 0; i < games.size(); i++) {
        outFile << "Title: " << games[i].title << endl;
        outFile << "Score: " << fixed << setprecision(6) << games[i].score << endl;
        
        if (i < games.size() - 1) {
            outFile << endl;
        }
    }
    
    outFile.close();
    cout << "Ranking saved to " << filename << endl;
}

// استخراج نام بازی از عنوان کامل
string extractGameName(const string& title) {
    // در برخی موارد نام بازی شامل عنوان‌های اضافی است که باید حذف شوند
    size_t colonPos = title.find(':');
    if (colonPos != string::npos) {
        return title.substr(0, colonPos);
    }
    
    // برخی از بازی‌ها دارای نام‌های طولانی هستند که باید کوتاه شوند
    size_t spacePos = title.find(' ');
    if (spacePos != string::npos && spacePos < 15) {
        return title.substr(0, spacePos);
    }
    
    return title;
}

int main(int argc, char* argv[]) {
    // بررسی آرگومان‌های ورودی
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <csv_file1> [csv_file2] [csv_file3] ..." << endl;
        return 1;
    }
    
    vector<GameData> allGames;
    
    // پردازش تمام فایل‌های ورودی
    for (int i = 1; i < argc; i++) {
        vector<GameData> games = processCSVFile(argv[i]);
        allGames.insert(allGames.end(), games.begin(), games.end());
    }
    
    if (allGames.empty()) {
        cerr << "No games were found in the input files." << endl;
        return 1;
    }
    
    // محاسبه امتیازها
    calculateScores(allGames);
    
    // مرتب‌سازی بازی‌ها بر اساس امتیاز
    sortGamesByScore(allGames);
    
    // ذخیره نتایج
    saveRankingToFile(allGames, "GameRanking.csv");
    
    return 0;
}