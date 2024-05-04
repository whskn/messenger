#include <string.h>
#include "validate.h"

bool msg_is_valid(void *msg_void_ptr, const int size)
{
    msg_t *msg = (msg_t *)msg_void_ptr;

    const int message_len = strnlen(msg->buffer, MAX_MESSAGE_LEN);

    // negative checks
    if (size < MIN_MSG_SIZE ||
        size != msg_size(msg) ||

        message_len == MAX_MESSAGE_LEN ||
        // Last char of the message must always be \0.
        // In this case message length == MAX_MESSAGE_SIZE,
        // that leaves no place for \0.

        msg->text_size < 2 ||
        message_len != msg->text_size - 1 ||
        // message len doesn't include \0, but text_size does

        msg->from_id < 1 ||
        msg->to_id < 1 ||

        msg->timestamp == 0)
    {
        return false;
    }
    return true;
}

bool passwd_filter(const int a)
{
    return a >= 32 && a <= 126 ? true : false;
}

bool passwd_is_valid(password_t p)
{
    int i = 0;
    for (; (size_t)i < sizeof(username_t) && p[i]; i++)
    {
        if (!passwd_filter(p[i]))
        {
            return false;
        }
    }
    if (i < 1 || p[i])
    {
        return false;
    }
    return true;
}

bool name_filter(const int a)
{
    if (!(a >= 48 && a <= 57) &&
        !(a >= 65 && a <= 90) &&
        !(a >= 97 && a <= 122))
    {
        return false;
    }
    return true;
}

bool name_is_valid(username_t name)
{
    int i = 0;
    for (; (size_t)i < sizeof(username_t) && name[i]; i++)
    {
        if (!name_filter(name[i]))
        {
            return false;
        }
    }
    if (i < 1 || name[i])
    {
        return false;
    }
    return true;
}
