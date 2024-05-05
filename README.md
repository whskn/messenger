# Messenger 📨

![1](https://github.com/whskn/messenger/assets/76423174/c0b442b6-7c17-4a87-b00f-47937c2b686a)

## What is this?
This is a nameless client-server messenger application written in C. It uses its own nameless protocol, and the client-side has a UI written in ncurses. I developed this project after finishing a System-Programming course at university to consolidate my knowledge and also have some fun.

## How does it work?
### Server 📻
When the server detects a pending connection, it creates a new thread to serve this connection. The serving thread mainly polls this connection's file descriptor. If a user sends a message, their serving thread receives it, checks it, and redirects it to the receiver (if they are online), or it writes the message to the database to send it later when the receiver goes online again.

The server stores connections in a so-called page with a pre-defined number of maximum connections. It has a page_mutex. After a user is connected and authenticated (auth_handler()), its connection (conn_t) has to be added to one of the page cells. Therefore, the page_mutex locks, and the server iterates through the cells to find empty ones. The new connection is then written into the first empty cell.

When user A sends a message to user B, the serving thread of user A locks page_mtx and iterates through the cells, trying to find a connection with the same user_id. If the user_id of one of the connections matches the searched one, the receiver's mutex (conn_mtx in conn_t) locks until the message is entirely sent. This is done because sending a message can involve multiple write() calls, which is obviously a vulnerable part in terms of mutual exclusion.

In short, when any connection on the page is being deleted/added or any of the worker threads search for the receiver, the page_mutex is locked to prevent data corruption. When information is being transferred, the receiver's mutex is locked since sending a message can involve multiple write() calls.

This page-oriented structure is very scalable; it's possible to allocate new pages to accommodate more connections (not realized yet). Since the page mutex affects only one page, the page's number of connections can be reduced for optimization.

![demo](https://github.com/whskn/messenger/assets/76423174/ad29e7b6-6307-4066-a0ea-2e459b44c89f)

### Client 💻
The client side is a single-threaded application. It polls two file descriptors: stdin and the connection's fd, and serves the interruptions.

The UI is written with the ncurses library and is very flexible and stable. It supports a couple of modes, among them it can switch (depending on the size of the window).

![resizing](https://github.com/whskn/messenger/assets/76423174/adfa037d-058c-4fd3-89fa-a6c7a27d42bb)

### Protocol 📜
The server and client use structures defined in proto.h for communication with each other. It's simple: one type of request/response - one structure. This model can be scaled to add new features very easily. Each such structure has a communication code (int cc) as the first argument. CC basically answers the question: "Why are you sending me this message?". Answers are: authentication (CC_AUTH), user request (CC_USER_RQS), message (CC_MESSAGE). But CC can be the same for a couple of structures. For example, both auth-request and auth-response structures have the same CC (CC_AUTH).

For the reason that the length of the data sent through the connection can vary, before sending any message/request, the integer-typed variable "size" is sent as the first 4 bytes before every data. This size variable is not part of any structure and therefore must not be thought of outside of net_send(), net_read() functions (in network.c files).

First, after the connection is established, the user needs to be authenticated: it sends auth_rqs_t to the server with its username and password. The server checks if the password matches and sends auth_res_t with either the user's id (to be used later for identification) or a rejection.

Before two users can communicate, they need to have each other's user_id. Therefore, there is another type of request, CC_USER_RQS. It uses structures: user_rqs_t, user_rsp_t.

After these previous steps are done, the users can finally send each other messages using msg_t structure and CC_MSG code.
