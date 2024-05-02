#define CREATE_CLI_TABLES \
    "CREATE TABLE IF NOT EXISTS contacts (    \
    user_id INTEGER PRIMARY KEY,          \
    username TEXT UNIQUE                  \
);                                        \
                                          \
CREATE TABLE IF NOT EXISTS messages (     \
    message_id INTEGER PRIMARY KEY,       \
    from_id INTEGER,                      \
    to_id INTEGER,                        \
    from_name TEXT,                       \
    timestamp INTEGER,                    \
    text_len INTEGER,                     \
    message TEXT,                          \
    FOREIGN KEY (from_id) REFERENCES contacts(user_id),  \
    FOREIGN KEY (to_id)   REFERENCES contacts(user_id) \
);"

#define PUSH "INSERT INTO \
messages(from_id, to_id, from_name, timestamp, text_len, message) \
SELECT ?, ?, ?, ?, ?, ?;"

#define PULL "SELECT * FROM messages WHERE from_id = ? \
ORDER BY timestamp ASC LIMIT 1;"

#define CHECK_CONTACT "SELECT COUNT(*) FROM contacts WHERE user_id = ?;"

#define DELETE_MSG "DELETE FROM messages WHERE message_id = ?;"

#define DELETE_CHAT "DELETE FROM contacts WHERE user_id = ?;"

#define DELETE_MESSAGES "DELETE FROM messages WHERE from_id = ?;"

#define COUNT_CHATS "SELECT COUNT(*) AS num_chats FROM contacts;"

#define PULL_CHATS "SELECT user_id, username FROM contacts;"

#define INSERT_CHAT "INSERT INTO contacts(user_id, username) VALUES (?, ?);"

#define GET_NEXT "SELECT * FROM messages WHERE from_id = ? OR to_id = ? \
ORDER BY timestamp DESC LIMIT 1 OFFSET ?;"

#define COUNT_ROWS "SELECT COUNT(*) AS count WHERE from_id = ? OR to_id = ?;"

#define TRANSACTION "BEGIN TRANSACTION"
#define COMMIT "COMMIT"
