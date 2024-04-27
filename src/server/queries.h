#define CREATE_CLI_TABLES                 \
"CREATE TABLE IF NOT EXISTS users (       \
    user_id INTEGER PRIMARY KEY,          \
    username TEXT UNIQUE,                 \
    password TEXT                         \
);                                        \
                                          \
CREATE TABLE IF NOT EXISTS messages (     \
    message_id INTEGER PRIMARY KEY,       \
    to_id INTEGER,                        \
    from_id INTEGER,                      \
    timestamp INTEGER,                    \
    text_size INTEGER,                    \
    message TEXT,                         \
    sent INTEGER,                         \
    FOREIGN KEY (from_id) REFERENCES users(user_id),  \
    FOREIGN KEY (to_id)   REFERENCES users(user_id) \
);"


#define NEW_MSG \
"INSERT INTO messages(from_id, to_id, timestamp, text_size, message, sent) \
VALUES (?, ?, ?, ?, ?, ?);"

#define NEW_USER "INSERT INTO users(username, password) VALUES (?, ?);"

#define GET_USER \
"SELECT user_id, username, password FROM users WHERE username = ? LIMIT 1;"

#define PULL_UNSENT             \
"SELECT m.message_id, m.to_id, m.from_id, m.timestamp, m.text_size, m.message, u.username \
FROM messages AS m \
JOIN users AS u ON m.from_id = u.user_id \
WHERE m.to_id = ? AND m.sent = 0 AND m.timestamp > ? \
ORDER BY m.timestamp ASC \
LIMIT 1;"

#define MARK_AS_SENT "UPDATE messages SET sent = 1 WHERE message_id = ?;"


#define DELETE_MSG "DELETE FROM messages WHERE message_id = ?;"


#define TRANSACTION "BEGIN TRANSACTION;"
#define COMMIT "COMMIT;"
