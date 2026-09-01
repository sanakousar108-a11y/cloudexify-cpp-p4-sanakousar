/*
    Library Management System
    CloudExify C++ Internship - Month 2, Project 4 (Final Project)

    Manages books, members, and issue/return records with fine
    calculation, and saves everything to disk so nothing is lost
    between runs.

    Concepts used: structs, vectors, pointers (* and &), file I/O, dates
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <limits>
#include <algorithm>
using namespace std;

// ---------- Structs ----------

struct Book {
    int bookID;
    string title;
    string author;
    string ISBN;
    int totalCopies;
    int availableCopies;
    double price;
};

struct Member {
    int memberID;
    string name;
    string email;
    string joinDate;
    int booksIssued;   // currently borrowed (not yet returned)
};

struct IssuedBook {
    int bookID;
    int memberID;
    string issueDate;
    string dueDate;
    double fineAmount;   // only meaningful once isReturned is true
    bool isReturned;
};

// ---------- Globals ----------

const string BOOKS_FILE = "books.dat";
const string MEMBERS_FILE = "members.dat";
const string ISSUED_FILE = "issued.dat";

const int LOAN_PERIOD_DAYS = 14;
const double DAILY_FINE_RATE = 5.0;   // Rs per day overdue

vector<Book> books;
vector<Member> members;
vector<IssuedBook> issuedBooks;

int nextBookID = 1;
int nextMemberID = 1001;

// ---------- Safe numeric input ----------
// A plain "cin >> x" leaves cin in a permanent fail state if the user
// types something that isn't a number, and every later read then fails
// too. This clears that state, discards the bad line, and re-prompts,
// and always eats the trailing newline so a later getline() doesn't see
// a leftover '\n'.
template <typename T>
T readNumber(const string& prompt) {
    T value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input, please enter a number." << endl;
    }
}

string readLine(const string& prompt) {
    string value;
    cout << prompt;
    getline(cin, value);
    return value;
}

// ---------- Date helpers ----------
// Dates are stored as plain "YYYY-MM-DD" strings so they're easy to
// print, easy to parse back, and sort correctly as text.

// tm_hour is fixed at noon when parsing so that a daylight-saving-time
// shift can't accidentally push the date itself to the day before/after.
time_t dateToTimeT(const string& dateISO) {
    tm timeinfo = {};
    int y, m, d;
    sscanf(dateISO.c_str(), "%d-%d-%d", &y, &m, &d);
    timeinfo.tm_year = y - 1900;
    timeinfo.tm_mon = m - 1;
    timeinfo.tm_mday = d;
    timeinfo.tm_hour = 12;
    timeinfo.tm_isdst = -1;
    return mktime(&timeinfo);
}

string timeTToDate(time_t t) {
    tm* timeinfo = localtime(&t);
    char buf[11];
    strftime(buf, sizeof(buf), "%Y-%m-%d", timeinfo);
    return string(buf);
}

string getCurrentDate() {
    time_t now = time(0);
    return timeTToDate(now);
}

string addDays(const string& dateISO, int days) {
    time_t t = dateToTimeT(dateISO) + static_cast<time_t>(days) * 86400;
    return timeTToDate(t);
}

// Positive = 'to' is later than 'from'
int daysBetween(const string& fromISO, const string& toISO) {
    double seconds = difftime(dateToTimeT(toISO), dateToTimeT(fromISO));
    return static_cast<int>(round(seconds / 86400.0));
}

string toLowerStr(const string& s) {
    string out = s;
    transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

// ---------- Find helpers (return pointers so callers can modify in place) ----------

Book* findBookByID(int id) {
    for (size_t i = 0; i < books.size(); i++) {
        if (books[i].bookID == id) {
            return &books[i];
        }
    }
    return nullptr;
}

Member* findMemberByID(int id) {
    for (size_t i = 0; i < members.size(); i++) {
        if (members[i].memberID == id) {
            return &members[i];
        }
    }
    return nullptr;
}

// ---------- Add book ----------
void addBook() {
    cout << "\n--- ADD NEW BOOK ---" << endl;

    Book newBook;
    newBook.bookID = nextBookID++;
    newBook.title = readLine("Title: ");
    newBook.author = readLine("Author: ");
    newBook.ISBN = readLine("ISBN: ");
    newBook.totalCopies = readNumber<int>("Total Copies: ");

    if (newBook.totalCopies <= 0) {
        cout << "Total copies must be at least 1! Book not added." << endl;
        nextBookID--;
        return;
    }

    newBook.price = readNumber<double>("Price (Rs): ");

    if (newBook.price < 0) {
        cout << "Price cannot be negative! Book not added." << endl;
        nextBookID--;
        return;
    }

    newBook.availableCopies = newBook.totalCopies;

    books.push_back(newBook);

    cout << "\nBook added successfully! Book ID: " << newBook.bookID << endl;
}

// ---------- Add member ----------
void addMember() {
    cout << "\n--- REGISTER NEW MEMBER ---" << endl;

    Member newMember;
    newMember.memberID = nextMemberID++;
    newMember.name = readLine("Name: ");
    newMember.email = readLine("Email: ");
    newMember.joinDate = getCurrentDate();
    newMember.booksIssued = 0;

    members.push_back(newMember);

    cout << "\nMember registered successfully! Member ID: " << newMember.memberID << endl;
}

// ---------- Issue a book to a member ----------
void issueBook() {
    if (books.empty() || members.empty()) {
        cout << "Add books and members first!" << endl;
        return;
    }

    int bookID = readNumber<int>("Book ID: ");
    int memberID = readNumber<int>("Member ID: ");

    // Use pointers to find and later modify the real records, not copies
    Book* book = findBookByID(bookID);
    Member* member = findMemberByID(memberID);

    if (book == nullptr || member == nullptr) {
        cout << "Book or Member not found!" << endl;
        return;
    }

    if (book->availableCopies <= 0) {
        cout << "No copies available!" << endl;
        return;
    }

    // Modify the real book/member objects through the pointers
    book->availableCopies--;
    member->booksIssued++;

    IssuedBook record;
    record.bookID = bookID;
    record.memberID = memberID;
    record.issueDate = getCurrentDate();
    record.dueDate = addDays(record.issueDate, LOAN_PERIOD_DAYS);
    record.fineAmount = 0;
    record.isReturned = false;

    issuedBooks.push_back(record);

    cout << "\nBook issued successfully!" << endl;
    cout << "Due date: " << record.dueDate << endl;
}

// ---------- Return a book ----------
void returnBook() {
    if (issuedBooks.empty()) {
        cout << "No books have been issued yet!" << endl;
        return;
    }

    int bookID = readNumber<int>("Book ID: ");
    int memberID = readNumber<int>("Member ID: ");

    // Find the matching outstanding (not yet returned) loan record
    IssuedBook* record = nullptr;
    for (size_t i = 0; i < issuedBooks.size(); i++) {
        if (issuedBooks[i].bookID == bookID &&
            issuedBooks[i].memberID == memberID &&
            !issuedBooks[i].isReturned) {
            record = &issuedBooks[i];
            break;
        }
    }

    if (record == nullptr) {
        cout << "No matching active loan found for that book and member!" << endl;
        return;
    }

    string today = getCurrentDate();
    int daysOverdue = daysBetween(record->dueDate, today);

    if (daysOverdue > 0) {
        record->fineAmount = daysOverdue * DAILY_FINE_RATE;
    } else {
        record->fineAmount = 0;
    }

    record->isReturned = true;

    Book* book = findBookByID(bookID);
    Member* member = findMemberByID(memberID);

    if (book != nullptr) {
        book->availableCopies++;
    }
    if (member != nullptr && member->booksIssued > 0) {
        member->booksIssued--;
    }

    cout << "\nBook returned successfully!" << endl;
    if (record->fineAmount > 0) {
        cout << "This book was " << daysOverdue << " day(s) overdue." << endl;
        cout << fixed << setprecision(2);
        cout << "Fine due: Rs " << record->fineAmount << endl;
    } else {
        cout << "Returned on time, no fine." << endl;
    }
}

// ---------- Search by title or ISBN ----------
void searchBook() {
    if (books.empty()) {
        cout << "No books in the library yet!" << endl;
        return;
    }

    string term = readLine("Enter title or ISBN (or part of it): ");
    string termLower = toLowerStr(term);

    bool found = false;
    for (const auto& b : books) {
        if (toLowerStr(b.title).find(termLower) != string::npos ||
            b.ISBN.find(term) != string::npos) {
            cout << "\nBook ID: " << b.bookID
                 << " | Title: " << b.title
                 << " | Author: " << b.author
                 << " | ISBN: " << b.ISBN
                 << " | Available: " << b.availableCopies << "/" << b.totalCopies << endl;
            found = true;
        }
    }

    if (!found) {
        cout << "No matching books found." << endl;
    }
}

// ---------- Display all books ----------
void viewAllBooks() {
    if (books.empty()) {
        cout << "No books in the library yet!" << endl;
        return;
    }

    cout << "\n--- ALL BOOKS ---" << endl;
    cout << left << setw(6) << "ID"
         << setw(28) << "Title"
         << setw(18) << "Author"
         << setw(12) << "Available"
         << setw(10) << "Price" << endl;
    cout << string(74, '-') << endl;

    for (const auto& b : books) {
        cout << left << setw(6) << b.bookID
             << setw(28) << b.title
             << setw(18) << b.author
             << setw(12) << (to_string(b.availableCopies) + "/" + to_string(b.totalCopies))
             << fixed << setprecision(2) << setw(10) << b.price << endl;
    }
}

// ---------- Display all members ----------
void viewMembers() {
    if (members.empty()) {
        cout << "No members registered yet!" << endl;
        return;
    }

    cout << "\n--- ALL MEMBERS ---" << endl;
    cout << left << setw(8) << "ID"
         << setw(20) << "Name"
         << setw(25) << "Email"
         << setw(12) << "Joined"
         << setw(10) << "Issued" << endl;
    cout << string(75, '-') << endl;

    for (const auto& m : members) {
        cout << left << setw(8) << m.memberID
             << setw(20) << m.name
             << setw(25) << m.email
             << setw(12) << m.joinDate
             << setw(10) << m.booksIssued << endl;
    }
}

// ---------- Full issue/return history for one member ----------
void memberHistory() {
    int memberID = readNumber<int>("Member ID: ");

    Member* member = findMemberByID(memberID);
    if (member == nullptr) {
        cout << "Member not found!" << endl;
        return;
    }

    cout << "\n--- HISTORY for " << member->name << " (ID " << memberID << ") ---" << endl;

    bool found = false;
    for (const auto& record : issuedBooks) {
        if (record.memberID == memberID) {
            Book* book = findBookByID(record.bookID);
            string title = (book != nullptr) ? book->title : "(unknown book)";

            cout << "Book: " << title
                 << " | Issued: " << record.issueDate
                 << " | Due: " << record.dueDate;

            if (record.isReturned) {
                cout << " | Returned"
                     << " | Fine: Rs " << fixed << setprecision(2) << record.fineAmount << endl;
            } else {
                cout << " | Still with member" << endl;
            }
            found = true;
        }
    }

    if (!found) {
        cout << "No borrowing history for this member." << endl;
    }
}

// ---------- Finance / statistics report ----------
void financeReport() {
    double inventoryValue = 0;
    int totalCopies = 0;
    int totalAvailable = 0;

    for (const auto& b : books) {
        inventoryValue += b.price * b.totalCopies;
        totalCopies += b.totalCopies;
        totalAvailable += b.availableCopies;
    }

    double finesCollected = 0;
    double finesOutstanding = 0;
    int currentlyIssued = 0;
    int overdueCount = 0;
    string today = getCurrentDate();

    for (const auto& record : issuedBooks) {
        if (record.isReturned) {
            finesCollected += record.fineAmount;
        } else {
            currentlyIssued++;
            int daysOverdue = daysBetween(record.dueDate, today);
            if (daysOverdue > 0) {
                overdueCount++;
                finesOutstanding += daysOverdue * DAILY_FINE_RATE;
            }
        }
    }

    cout << fixed << setprecision(2);
    cout << "\n--- LIBRARY FINANCE & STATISTICS REPORT ---" << endl;
    cout << "Total books (titles):        " << books.size() << endl;
    cout << "Total copies owned:          " << totalCopies << endl;
    cout << "Copies currently available:  " << totalAvailable << endl;
    cout << "Total inventory value:       Rs " << inventoryValue << endl;
    cout << "Registered members:          " << members.size() << endl;
    cout << "Books currently on loan:     " << currentlyIssued << endl;
    cout << "Overdue loans right now:     " << overdueCount << endl;
    cout << "Fines collected (returned):  Rs " << finesCollected << endl;
    cout << "Fines outstanding (overdue): Rs " << finesOutstanding << endl;
}

// ---------- File I/O ----------

vector<string> splitLine(const string& line, char delim) {
    vector<string> parts;
    stringstream ss(line);
    string item;
    while (getline(ss, item, delim)) {
        parts.push_back(item);
    }
    return parts;
}

void saveLibraryData() {
    ofstream booksFile(BOOKS_FILE);
    if (booksFile) {
        booksFile << books.size() << endl;
        for (const auto& b : books) {
            booksFile << b.bookID << "|" << b.title << "|" << b.author << "|"
                      << b.ISBN << "|" << b.totalCopies << "|" << b.availableCopies
                      << "|" << b.price << endl;
        }
        booksFile.close();
    } else {
        cout << "Error: could not save books!" << endl;
    }

    ofstream membersFile(MEMBERS_FILE);
    if (membersFile) {
        membersFile << members.size() << endl;
        for (const auto& m : members) {
            membersFile << m.memberID << "|" << m.name << "|" << m.email << "|"
                        << m.joinDate << "|" << m.booksIssued << endl;
        }
        membersFile.close();
    } else {
        cout << "Error: could not save members!" << endl;
    }

    ofstream issuedFile(ISSUED_FILE);
    if (issuedFile) {
        issuedFile << issuedBooks.size() << endl;
        for (const auto& r : issuedBooks) {
            issuedFile << r.bookID << "|" << r.memberID << "|" << r.issueDate << "|"
                       << r.dueDate << "|" << r.fineAmount << "|" << (r.isReturned ? 1 : 0) << endl;
        }
        issuedFile.close();
    } else {
        cout << "Error: could not save issue records!" << endl;
    }

    cout << "Data saved!" << endl;
}

void loadLibraryData() {
    ifstream booksFile(BOOKS_FILE);
    if (booksFile) {
        int count;
        booksFile >> count;
        booksFile.ignore();
        books.clear();

        for (int i = 0; i < count; i++) {
            string line;
            getline(booksFile, line);
            vector<string> parts = splitLine(line, '|');
            if (parts.size() < 7) continue;

            Book b;
            b.bookID = stoi(parts[0]);
            b.title = parts[1];
            b.author = parts[2];
            b.ISBN = parts[3];
            b.totalCopies = stoi(parts[4]);
            b.availableCopies = stoi(parts[5]);
            b.price = stod(parts[6]);

            books.push_back(b);

            if (b.bookID >= nextBookID) {
                nextBookID = b.bookID + 1;
            }
        }
        booksFile.close();
    }

    ifstream membersFile(MEMBERS_FILE);
    if (membersFile) {
        int count;
        membersFile >> count;
        membersFile.ignore();
        members.clear();

        for (int i = 0; i < count; i++) {
            string line;
            getline(membersFile, line);
            vector<string> parts = splitLine(line, '|');
            if (parts.size() < 5) continue;

            Member m;
            m.memberID = stoi(parts[0]);
            m.name = parts[1];
            m.email = parts[2];
            m.joinDate = parts[3];
            m.booksIssued = stoi(parts[4]);

            members.push_back(m);

            if (m.memberID >= nextMemberID) {
                nextMemberID = m.memberID + 1;
            }
        }
        membersFile.close();
    }

    ifstream issuedFile(ISSUED_FILE);
    if (issuedFile) {
        int count;
        issuedFile >> count;
        issuedFile.ignore();
        issuedBooks.clear();

        for (int i = 0; i < count; i++) {
            string line;
            getline(issuedFile, line);
            vector<string> parts = splitLine(line, '|');
            if (parts.size() < 6) continue;

            IssuedBook r;
            r.bookID = stoi(parts[0]);
            r.memberID = stoi(parts[1]);
            r.issueDate = parts[2];
            r.dueDate = parts[3];
            r.fineAmount = stod(parts[4]);
            r.isReturned = (stoi(parts[5]) != 0);

            issuedBooks.push_back(r);
        }
        issuedFile.close();
    }
}

// ---------- Menu ----------
void showMenu() {
    cout << "\n===================================" << endl;
    cout << "     LIBRARY MANAGEMENT SYSTEM" << endl;
    cout << "===================================" << endl;
    cout << "1. Add Book" << endl;
    cout << "2. Add Member" << endl;
    cout << "3. Issue Book" << endl;
    cout << "4. Return Book" << endl;
    cout << "5. Search Book" << endl;
    cout << "6. View All Books" << endl;
    cout << "7. View All Members" << endl;
    cout << "8. Member Borrowing History" << endl;
    cout << "9. Finance & Statistics Report" << endl;
    cout << "10. Save & Exit" << endl;
    cout << "===================================" << endl;
}

int main() {
    loadLibraryData();

    cout << "Welcome to the Library Management System!" << endl;
    if (!books.empty() || !members.empty()) {
        cout << "(Loaded " << books.size() << " book(s) and "
             << members.size() << " member(s) from file.)" << endl;
    }

    int choice;
    do {
        showMenu();
        choice = readNumber<int>("Choose an option: ");

        switch (choice) {
            case 1:
                addBook();
                break;
            case 2:
                addMember();
                break;
            case 3:
                issueBook();
                break;
            case 4:
                returnBook();
                break;
            case 5:
                searchBook();
                break;
            case 6:
                viewAllBooks();
                break;
            case 7:
                viewMembers();
                break;
            case 8:
                memberHistory();
                break;
            case 9:
                financeReport();
                break;
            case 10:
                saveLibraryData();
                cout << "\nGoodbye!" << endl;
                break;
            default:
                cout << "Invalid option, try again." << endl;
        }
    } while (choice != 10);

    return 0;
}
