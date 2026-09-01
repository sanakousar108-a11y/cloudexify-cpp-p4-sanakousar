# cloudexify-cpp-p4-sanakousar
## What it does

- Add books (title, author, ISBN, copies, price)
- Register members
- Issue a book to a member (checks copies are available)
- Return a book (automatically calculates a fine if it's overdue)
- Search books by title or ISBN
- View all books / all members in formatted tables
- View a member's full borrowing history
- Generate a finance & statistics report (inventory value, fines collected,
  fines currently outstanding, overdue count, etc.)
- Save/load everything from `books.dat`, `members.dat`, and `issued.dat`

## Concepts I practiced

- **Pointers** — `findBookByID()` and `findMemberByID()` return pointers
  into the real `vector`, not copies, so `issueBook()` and `returnBook()`
  can update `book->availableCopies` and `member->booksIssued` directly.
  `returnBook()` does the same thing with a pointer into the `issuedBooks`
  vector so it can mark that specific loan as returned.
- **Structs** — `Book`, `Member`, and `IssuedBook` model the three kinds
  of data the system tracks.
- **File I/O** — three separate pipe-delimited files, loaded back on
  startup the same way Project 3's banking system did.
- **Date math** — dates are stored as `YYYY-MM-DD` strings and converted
  to `time_t` with `mktime()` to calculate due dates and days overdue.

## Challenges I faced

The hardest part was the date/fine logic, not the pointers. A couple of
things that tripped me up:

- **Overdue calculation** needs a real day count, not just string
  comparison, so `daysBetween()` converts both dates to `time_t` and uses
  `difftime()` divided by seconds-per-day.
- **Daylight saving time** can shift a `time_t` by an hour, which was
  occasionally rounding a day count up or down by one. Fixing the parsed
  time to noon (`tm_hour = 12`) instead of midnight avoided that, since a
  ±1 hour DST shift can't cross midnight and change the date.
- **Matching a return to the right loan** — a member could issue the same
  book twice over time, so `returnBook()` specifically looks for the
  *not-yet-returned* record for that book/member pair, rather than just
  the first record it finds.
- I also reused the `readNumber<T>()` safe-input helper from the banking
  project, since the same "letters typed at a numeric prompt hang the
  program forever" bug applies here too.

## Testing

| Test case | Result |
|---|---|
| Add 5 books with different titles | ✅ All saved with unique IDs |
| Add 3 library members | ✅ All saved with unique IDs |
| Issue book that exists and has copies | ✅ Available copies decrease by 1 |
| Issue book with no available copies | ✅ "No copies available!" |
| Issue book to non-existing member | ✅ "Book or Member not found!" |
| Return a book | ✅ Copies increase, member's issued count decreases |
| Calculate fine for overdue book | ✅ Verified: 6 days overdue × Rs 5/day = Rs 30 exactly |
| Return on time | ✅ No fine charged |
| Search for book by title | ✅ Found and displayed (case-insensitive) |
| View all books in formatted table | ✅ All books shown |
| Save and reload library data | ✅ All data preserved across restart |



## My 4 CloudExify C++ projects
Project 1:[Number Gessing Game](https://github.com/sanakousar108-a11y/cloudexify-cpp-p1-sanakousar)
Project 2:[Student Record System](https://github.com/sanakousar108-a11y/cloudexify-cpp-p2-sanakousar)
Project 3:[Simple Banking System](https://github.com/sanakousar108-a11y/cloudexify-cpp-p3-sanakousar)
