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

// ����� �������
struct Product {
    string name;
    string barcode;
    double price;
};

// ����� ����� ���� (����� ����� ���� ������� ����� std::map(product, price), ���� ���� ������ ���������� ���������� ID)
struct ReceiptItem {
    Product product;
    int quantity;
};

// ����� ������� ������, ��� �������� ���������� ������
enum PaymentMethod { CASH, CARD };

// ����� ����
struct Receipt {
    vector<ReceiptItem> items;
    PaymentMethod payment;
    double total;
    double amountPaid;
    double change;
    time_t timestamp;
};

// ����� �����
struct Shift {
    string cashierName;
    time_t openTime;
    time_t closeTime;
    double initialCash;
    double currentCash;
    map<PaymentMethod, double> paymentStats;
    vector<Receipt> receipts;
};

// ����� ����������
vector<Product> products;
Shift currentShift;
bool isShiftOpen = false;

// �������
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
            cout << "\n������� �������� (���, ���, ���, ���): ";
            getline(cin, command);

            if (command == "���") {
                openShift();
            } else if (command == "���") {
                closeShift();
            } else if (command == "���") {
                if (isShiftOpen) {
                    processSale();
                } else {
                    cerr << "������: ��� �������� �����!" << endl;
                }
            } else if (command == "���") {
                if (isShiftOpen) {
                    cerr << "������: ��������� ������� ����� ����� �������!" << endl;
                } else {
                    cout << "�������� ������." << endl;
                    return 0;
                }
            } else {
                cout << "����������� ��������, ���������� ��� ���." << endl;
            }
        }
    } catch (const exception& e) {
        cerr << "����������� ������: " << e.what() << endl;
        return 1;
    }
}

// �������� ������ �� csv ������� 
// ��������,�����-���, ����:
// ������,123456789,2.50

void loadProducts(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("������ �������� ����� ������� products.csv");
    }

    string line;
    int lineNumber = 0;
    int loadedCount = 0;
    
    while (getline(file, line)) {
        lineNumber++;
        stringstream ss(line);
        string name, barcode, priceStr;
        
        // �������� ��������
        if (!getline(ss, name, ',') || !getline(ss, barcode, ',') || !getline(ss, priceStr, ',')) {
            cerr << "������ � ������ " << lineNumber << endl;
            continue;
        }

        // �������� �����
        string extra;
        if (getline(ss, extra, ',')) {
            cerr << "������ � ������ " << lineNumber << endl;
            continue;
        }

        // �������� �����-�����
        if (barcode.length() != 13) {
            cerr << "������ � ������ " << lineNumber << endl;
            continue;
        }
        if (!all_of(barcode.begin(), barcode.end(), ::isdigit)) {
            cerr << "������ � ������ " << lineNumber << endl;
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
            cerr << "������ � ������ " << lineNumber << endl;
            continue;
        }
        catch (const out_of_range&) {
            cerr << "������ � ������ " << lineNumber << endl;
            continue;
        }
    }

    cout << "��������� " << loadedCount << " ������� �� " << lineNumber << " �����" << endl;
    if (loadedCount != lineNumber) {
        cerr << "��������: " << (lineNumber - loadedCount) << " ����� ��������� ������ � ���� ���������" << endl;
    }
}

void printWelcome() {
    cout << "==============================" << endl;
    cout << "     ������� ����� �������"     << endl;
    cout << "==============================" << endl;
    cout << "��������:" << endl;
    cout << "��� - ������� �����" << endl;
    cout << "��� - ������� �����" << endl;
    cout << "��� - ���������� �������" << endl;
    cout << "��� - ����� �� ���������" << endl;
    cout << "==============================" << endl;
}

// ���� ��������
double getValidCashInput() {
    double cash;
    while (true) {
        std::cin >> cash;

        if (std::cin.fail()) {
            std::cin.clear(); 
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
            std::cout << "������: ������� �����. ���������� ��� ���.\n";
        }
        // �������� �� ������������� ��������
        else if (cash < 0) {
            std::cout << "������: ����� �� ����� ���� �������������. ���������� ��� ���.\n";
        }
        else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
            return cash; 
        }
    }
}

// ������� �����
void openShift() {
    try {
        if (isShiftOpen) {
            cerr << "������: ����� ��� �������!" << endl;
            return;
        }

        cout << "���� ���: ";
        getline(cin, currentShift.cashierName);

        cout << "�������� � �����: ";
        currentShift.initialCash = getValidCashInput();
        currentShift.currentCash = currentShift.initialCash;

        currentShift.openTime = time(nullptr);
        currentShift.paymentStats[CASH] = 0;
        currentShift.paymentStats[CARD] = 0;
        currentShift.receipts.clear();

        isShiftOpen = true;

        cout << "\n����� �������: " << currentShift.cashierName << endl;
        cout << "� ���������: " << fixed << setprecision(2) << currentShift.initialCash << endl;
        cout << "����� ��������: " << ctime(&currentShift.openTime);

    } catch (const exception& e) {
        cerr << "����������� ������: " << e.what() << endl;
        return;
    }
}

// ������� �����
void closeShift() {
    if (!isShiftOpen) {
        cerr << "������: ��� �������� �����!" << endl;
        return;
    }

    currentShift.closeTime = time(nullptr);
    isShiftOpen = false;

    printShiftReport();
}

// �������� �� ������ �����
int getValidQuantityInput() {
    int quantity;
    while (true) {
        std::cout << "������� ����������: ";
        std::cin >> quantity;

        if (std::cin.fail()) {
            std::cin.clear(); 
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "������! ������� ����� �����.\n";
        }
        else if (quantity <= 0) {
            std::cout << "������! ���������� ������ ���� ������ 0.\n";
        }
        else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return quantity; 
        }
    }
}

// ��������� �������
void processSale() {
    try {
        Receipt receipt;
        receipt.timestamp = time(nullptr);
        receipt.total = 0;
        receipt.items.clear();

        cout << "\n������ �������. ������� '���' ����� ���������� ���� �������." << endl;

        while (true) {
            cout << "������� ��� ������ ��� 13-�������� �������� �����-����: ";
            string query;
            getline(cin, query);

            if (query == "���") {
                break;
            }

            Product* product = findProduct(query);
            if (!product) {
                cout << "����� �� ������. ���������� ��� ���." << endl;
                continue;
            }

            cout << "������ �����: " << product->name << " (" << product->barcode << ") - " 
                << fixed << setprecision(2) << product->price << endl;

            int quantity = getValidQuantityInput();

            ReceiptItem item;
            item.product = *product;
            item.quantity = quantity;
            receipt.items.push_back(item);

            receipt.total += product->price * quantity;
            cout << "������� �����: " << fixed << setprecision(2) << receipt.total << endl;
        }

        if (receipt.items.empty()) {
            cout << "������ ������� - ��������� 0 �������." << endl;
            return;
        }

        cout << "\n�����: " << fixed << setprecision(2) << receipt.total << endl;
        
        while (true) {
            cout << "������ (���/���/���): ";
            string paymentMethod;
            getline(cin, paymentMethod);

            transform(paymentMethod.begin(), paymentMethod.end(), paymentMethod.begin(), ::tolower);
            if (paymentMethod == "���") {
                receipt.payment = CASH;
                cout << "������� ���������� �����: ";
                receipt.amountPaid = getValidCashInput();
    
                if (receipt.amountPaid < receipt.total) {
                    cerr << "������: ������������� ����� ��� ������!" << endl;
                    continue;
                } else {
                    receipt.change = receipt.amountPaid - receipt.total;

                    if (receipt.change > currentShift.currentCash) {
                        cerr << "������: � ����� ������������ ����� ��� �����!" << endl;

                        cerr << "������� '��', ����� �� ����� ��������� �������, ��� ����� ������ ������ ��� ������" << endl;
                        string cancelation;
                        getline(cin, cancelation);
                        if (cancelation != "��") {
                            continue;
                        } 

                    } 
                    currentShift.currentCash += receipt.total;
                    break;
                }
    
            } else if (paymentMethod == "���") {
                receipt.payment = CARD;
                receipt.amountPaid = receipt.total;
                receipt.change = 0;
                break;
            } else if (paymentMethod == "���") {
                cout << "������ �������.";
                return;
            } else {
                cerr << "������: ������������ ����� ������!" << endl;
            }
        }
        

        currentShift.paymentStats[receipt.payment] += receipt.total;
        currentShift.receipts.push_back(receipt);

        // ������ ����
        printReceipt(receipt);
    } catch (const exception& e) {
        cerr << "����������� ������: " << e.what() << endl;
        return;
    }
}

// ������ ����
void printReceipt(const Receipt& receipt) {
    cout << "\n============ ��� ============" << endl;
    cout << "����: " << ctime(&receipt.timestamp);
    cout << "������:" << endl;

    for (const auto& item : receipt.items) {
        cout << setw(10) << left << item.product.name.substr(0, 20) 
             << setw(4) << right << item.quantity << " x " 
             << setw(8) << fixed << setprecision(2) << item.product.price 
             << " = " << setw(8) << item.product.price * item.quantity << endl;
    }

    cout << "\n�����: " << setw(20) << fixed << setprecision(2) << receipt.total << endl;
    cout << "������: " << (receipt.payment == CASH ? "��������" : "�����") << endl;
    
    if (receipt.payment == CASH) {
        cout << "�������: " << setw(24) << fixed << setprecision(2) << receipt.amountPaid << endl;
        cout << "�����: " << setw(25) << fixed << setprecision(2) << receipt.change << endl;
    }

    cout << "============================" << endl;
}

// ����� �������
Product* findProduct(const string& query) {
    for (auto& product : products) {
        if (product.barcode == query || (product.name.compare(query) == 0)) {
            return &product;
        }
    }
    return nullptr;
}

// ����� � �����
void printShiftReport() {
    cout << "\n======== ����� � ����� ========" << endl;
    cout << "������: " << currentShift.cashierName << endl;
    cout << "����� �������: " << ctime(&currentShift.openTime);
    cout << "����� �������: " << ctime(&currentShift.closeTime);
    cout << "����������� ��������: " << fixed << setprecision(2) << currentShift.initialCash << endl;
    cout << "������� ���������: " << fixed << setprecision(2) << currentShift.paymentStats[CASH] << endl;
    cout << "������� ������: " << fixed << setprecision(2) << currentShift.paymentStats[CARD] << endl;
    cout << "�����: " << fixed << setprecision(2) 
         << (currentShift.paymentStats[CASH] + currentShift.paymentStats[CARD]) << endl;
    cout << "��������� ��������: " << fixed << setprecision(2) 
         << (currentShift.initialCash + currentShift.paymentStats[CASH]) << endl;
    cout << "��������: " << fixed << setprecision(2) << currentShift.currentCash << endl;
    cout << "���������� �����: " << currentShift.receipts.size() << endl;
    cout << "================================" << endl;
}