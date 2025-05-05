#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <limits> 
#include <clocale>
#ifdef _WIN32
    #include <Windows.h>
#endif
using namespace std;

// Класс товаров
struct Product {
    string name;
    string barcode;
    double price;
};

// Класс части чека (может можно было сделать через std::map(product, price), если дать товару уникальные порядковые ID)
struct ReceiptItem {
    Product product;
    int quantity;
};

// Выбор способа оплаты, для простого добавления других
enum PaymentMethod { CASH, CARD };

// Класс чека
struct Receipt {
    vector<ReceiptItem> items;
    PaymentMethod payment;
    double total;
    double amountPaid;
    double change;
    time_t timestamp;
};

// Класс смены
struct Shift {
    string cashierName;
    time_t openTime;
    time_t closeTime;
    double initialCash;
    double currentCash;
    map<PaymentMethod, double> paymentStats;
    vector<Receipt> receipts;
};

// Общие переменные
vector<Product> products;
Shift currentShift;
bool isShiftOpen = false;

// Функции
void loadProducts(const string& filename);
void printWelcome();
void openShift();
void closeShift();
void processSale();
void printReceipt(const Receipt& receipt);
Product* findProduct(const string& query);
void printShiftReport();

int main() {
    try {
        #ifdef _WIN32
            SetConsoleCP(1251);
            SetConsoleOutputCP(1251);
            setlocale(LC_ALL, "Russian");
        #endif
        loadProducts("products.csv");
        printWelcome();

        string command;
        while (true) {
            cout << "\nВведите комманду (отк, зкр, прд, вхд): ";
            getline(cin, command);

            if (command == "отк") {
                openShift();
            } else if (command == "зкр") {
                closeShift();
            } else if (command == "прд") {
                if (isShiftOpen) {
                    processSale();
                } else {
                    cerr << "Ошибка: Нет открытой смены!" << endl;
                }
            } else if (command == "вхд") {
                if (isShiftOpen) {
                    cerr << "Ошибка: Требуется закрыть смену перед выходом!" << endl;
                } else {
                    cout << "Закрытие сессии." << endl;
                    return 0;
                }
            } else {
                cout << "Неизвестная комманда, попробуйте ещё раз." << endl;
            }
        }
    } catch (const exception& e) {
        cerr << "Критическая ошибка: " << e.what() << endl;
        return 1;
    }
}

// Загрузка данных из csv формата 
// Название,штрих-код, цена:
// Молоко,123456789,2.50

void loadProducts(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Ошибка открытия файла товаров products.csv");
    }

    string line;
    int lineNumber = 0;
    int loadedCount = 0;
    
    while (getline(file, line)) {
        lineNumber++;
        stringstream ss(line);
        string name, barcode, priceStr;
        
        // Проверка столбцов
        if (!getline(ss, name, ',') || !getline(ss, barcode, ',') || !getline(ss, priceStr, ',')) {
            cerr << "Ошибка в строке " << lineNumber << endl;
            continue;
        }

        // Проверка строк
        string extra;
        if (getline(ss, extra, ',')) {
            cerr << "Ошибка в строке " << lineNumber << endl;
            continue;
        }

        // Проверка штрих-кодов
        if (barcode.length() != 13) {
            cerr << "Ошибка в строке " << lineNumber << endl;
            continue;
        }
        if (!all_of(barcode.begin(), barcode.end(), ::isdigit)) {
            cerr << "Ошибка в строке " << lineNumber << endl;
            continue;
        }

        try {
            double price = stod(priceStr);
            
            Product product;
            product.name = name;
            product.barcode = barcode;
            product.price = price;
            
            products.push_back(product);
            loadedCount++;
        } 
        catch (const invalid_argument&) {
            cerr << "Ошибка в строке " << lineNumber << endl;
            continue;
        }
        catch (const out_of_range&) {
            cerr << "Ошибка в строке " << lineNumber << endl;
            continue;
        }
    }

    cout << "Загружено " << loadedCount << " товаров из " << lineNumber << " строк" << endl;
    if (loadedCount != lineNumber) {
        cerr << "Внимание: " << (lineNumber - loadedCount) << " строк содержали ошибки и были пропущены" << endl;
    }
}

void printWelcome() {
    cout << "==============================" << endl;
    cout << "     Рабочее место кассира"     << endl;
    cout << "==============================" << endl;
    cout << "Комманды:" << endl;
    cout << "отк - Открыть смену" << endl;
    cout << "зкр - Закрыть смену" << endl;
    cout << "прд - Произвести продажу" << endl;
    cout << "вхд - Выход из программы" << endl;
    cout << "==============================" << endl;
}

// Ввод наличных
double getValidCashInput() {
    double cash;
    while (true) {
        std::cin >> cash;

        if (std::cin.fail()) {
            std::cin.clear(); 
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
            std::cout << "Ошибка: Введите число. Попробуйте ещё раз.\n";
        }
        // Проверка на отрицательное значение
        else if (cash < 0) {
            std::cout << "Ошибка: Сумма не может быть отрицательной. Попробуйте ещё раз.\n";
        }
        else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
            return cash; 
        }
    }
}

// Открыть смену
void openShift() {
    try {
        if (isShiftOpen) {
            cerr << "Ошибка: Смена уже открыта!" << endl;
            return;
        }

        cout << "Ваше имя: ";
        getline(cin, currentShift.cashierName);

        cout << "Наличных в кассе: ";
        currentShift.initialCash = getValidCashInput();
        currentShift.currentCash = currentShift.initialCash;

        currentShift.openTime = time(nullptr);
        currentShift.paymentStats[CASH] = 0;
        currentShift.paymentStats[CARD] = 0;
        currentShift.receipts.clear();

        isShiftOpen = true;

        cout << "\nСмена открыта: " << currentShift.cashierName << endl;
        cout << "С наличными: " << fixed << setprecision(2) << currentShift.initialCash << endl;
        cout << "Время открытия: " << ctime(&currentShift.openTime);

    } catch (const exception& e) {
        cerr << "Критическая ошибка: " << e.what() << endl;
        return;
    }
}

// Закрыть смену
void closeShift() {
    if (!isShiftOpen) {
        cerr << "Ошибка: Нет открытой смены!" << endl;
        return;
    }

    currentShift.closeTime = time(nullptr);
    isShiftOpen = false;

    printShiftReport();
}

// Проверка на ошибку ввода
int getValidQuantityInput() {
    int quantity;
    while (true) {
        std::cout << "Введите количество: ";
        std::cin >> quantity;

        if (std::cin.fail()) {
            std::cin.clear(); 
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Ошибка! Введите целое число.\n";
        }
        else if (quantity <= 0) {
            std::cout << "Ошибка! Количество должно быть больше 0.\n";
        }
        else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return quantity; 
        }
    }
}

// Обработка покупки
void processSale() {
    try {
        Receipt receipt;
        receipt.timestamp = time(nullptr);
        receipt.total = 0;
        receipt.items.clear();

        cout << "\nНачало продажи. Введите 'окч' после добавления всех товаров." << endl;

        while (true) {
            cout << "Введите имя товара или 13-циферное значение штрих-кода: ";
            string query;
            getline(cin, query);

            if (query == "окч") {
                break;
            }

            Product* product = findProduct(query);
            if (!product) {
                cout << "Товар не найден. Попробуйте ещё раз." << endl;
                continue;
            }

            cout << "Найден товар: " << product->name << " (" << product->barcode << ") - " 
                << fixed << setprecision(2) << product->price << endl;

            int quantity = getValidQuantityInput();

            ReceiptItem item;
            item.product = *product;
            item.quantity = quantity;
            receipt.items.push_back(item);

            receipt.total += product->price * quantity;
            cout << "Текущая сумма: " << fixed << setprecision(2) << receipt.total << endl;
        }

        if (receipt.items.empty()) {
            cout << "Отмена продажи - добавлено 0 товаров." << endl;
            return;
        }

        cout << "\nСумма: " << fixed << setprecision(2) << receipt.total << endl;
        
        while (true) {
            cout << "Оплата (нал/крт/отм): ";
            string paymentMethod;
            getline(cin, paymentMethod);

            transform(paymentMethod.begin(), paymentMethod.end(), paymentMethod.begin(), ::tolower);
            if (paymentMethod == "нал") {
                receipt.payment = CASH;
                cout << "Введите полученную сумму: ";
                receipt.amountPaid = getValidCashInput();
    
                if (receipt.amountPaid < receipt.total) {
                    cerr << "Ошибка: Недостаточная сумма для оплаты!" << endl;
                    continue;
                } else {
                    receipt.change = receipt.amountPaid - receipt.total;

                    if (receipt.change > currentShift.currentCash) {
                        cerr << "Ошибка: В кассе недостаточно денег для сдачи!" << endl;

                        cerr << "Введите 'ок', чтобы всё равно завершить покупку, или любую другую строку для отмены" << endl;
                        string cancelation;
                        getline(cin, cancelation);
                        if (cancelation != "ок") {
                            continue;
                        } 

                    } 
                    currentShift.currentCash += receipt.total;
                    break;
                }
    
            } else if (paymentMethod == "крт") {
                receipt.payment = CARD;
                receipt.amountPaid = receipt.total;
                receipt.change = 0;
                break;
            } else if (paymentMethod == "отм") {
                cout << "Отмена покупки.";
                return;
            } else {
                cerr << "Ошибка: Неправильный метод оплаты!" << endl;
            }
        }
        

        currentShift.paymentStats[receipt.payment] += receipt.total;
        currentShift.receipts.push_back(receipt);

        // Печать чека
        printReceipt(receipt);
    } catch (const exception& e) {
        cerr << "Критическая ошибка: " << e.what() << endl;
        return;
    }
}

// Печать чека
void printReceipt(const Receipt& receipt) {
    cout << "\n============ ЧЕК ============" << endl;
    cout << "Дата: " << ctime(&receipt.timestamp);
    cout << "Товары:" << endl;

    for (const auto& item : receipt.items) {
        cout << setw(10) << left << item.product.name.substr(0, 20) 
             << setw(4) << right << item.quantity << " x " 
             << setw(8) << fixed << setprecision(2) << item.product.price 
             << " = " << setw(8) << item.product.price * item.quantity << endl;
    }

    cout << "\nСУММА: " << setw(20) << fixed << setprecision(2) << receipt.total << endl;
    cout << "Оплата: " << (receipt.payment == CASH ? "Наличные" : "Карта") << endl;
    
    if (receipt.payment == CASH) {
        cout << "Принято: " << setw(24) << fixed << setprecision(2) << receipt.amountPaid << endl;
        cout << "Сдача: " << setw(25) << fixed << setprecision(2) << receipt.change << endl;
    }

    cout << "============================" << endl;
}

// Поиск товаров
Product* findProduct(const string& query) {
    for (auto& product : products) {
        if (product.barcode == query || (product.name.compare(query) == 0)) {
            return &product;
        }
    }
    return nullptr;
}

// Отчет о смене
void printShiftReport() {
    cout << "\n======== Отчет о смене ========" << endl;
    cout << "Кассир: " << currentShift.cashierName << endl;
    cout << "Смена открыта: " << ctime(&currentShift.openTime);
    cout << "Смена закрыта: " << ctime(&currentShift.closeTime);
    cout << "Изначальные наличные: " << fixed << setprecision(2) << currentShift.initialCash << endl;
    cout << "Продажи наличными: " << fixed << setprecision(2) << currentShift.paymentStats[CASH] << endl;
    cout << "Продажи картой: " << fixed << setprecision(2) << currentShift.paymentStats[CARD] << endl;
    cout << "Общее: " << fixed << setprecision(2) 
         << (currentShift.paymentStats[CASH] + currentShift.paymentStats[CARD]) << endl;
    cout << "Ожидаемые наличные: " << fixed << setprecision(2) 
         << (currentShift.initialCash + currentShift.paymentStats[CASH]) << endl;
    cout << "Наличные: " << fixed << setprecision(2) << currentShift.currentCash << endl;
    cout << "Количество чеков: " << currentShift.receipts.size() << endl;
    cout << "================================" << endl;
}