#include <string.h>

#include "validate.h"
#include "../message.h"

/**
 * Checks message for validity.
 * @param msg where the message is stored (msg_t* casted to void*)
 * @param size of msg excluding unused bytes in buffer
 * 
 * @return weather the message valid or not
*/
bool msg_is_valid(void* msg_void_ptr, const int size) {
    msg_t* msg = (msg_t*)msg_void_ptr;

    int message_len = strnlen(msg->buffer, MAX_MESSAGE_SIZE);
    int from_len = strnlen(msg->names.from, sizeof(username_t));
    int to_len   = strnlen(msg->names.to,   sizeof(username_t));

    // negative checks
    if (size < MIN_MSG_SIZE || 
        size != msg_size(msg) ||
        
        message_len == MAX_MESSAGE_SIZE ||
        // Last char of the message must always be \0. 
        // In this case message length == MAX_MESSAGE_SIZE, 
        // that leaves no place for \0.
        
        message_len < 1 ||
        msg->text_size < 2 ||
        // message len doesn't include \0, but text_size does

        from_len < 1 ||
        from_len > (int)sizeof(username_t) - 1 ||
        to_len < 1 ||
        to_len > (int)sizeof(username_t) - 1 ||
        // names are also C-strings, last char must be \0

        msg->timestamp == 0) {
            return false;
    } 
    return true;
}