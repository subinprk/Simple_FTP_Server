# Simple FTP Server & Client in C

## 📌 Overview

This project implements a simple FTP-style file sharing system using TCP sockets in C. It consists of:

* A **multi-client server** that supports basic chat and file operations.
* A **client** capable of connecting to the server, authenticating, and participating in file exchange or communication.

> 개발 목적: 소켓 프로그래밍 학습 및 기본적인 FTP 동작 원리 이해.

## 📁 Files

* `clipingup.c` — FTP **client** source code
* `hw7.c` — FTP **server** source code
* `14주숙제.docx` — 기술 문서 (구현 설명 포함)

## ⚙️ Features

### Server (`hw7.c`)

* Accepts multiple clients using `select()`
* Handles connection lifecycle via `enum state`
* Client state management:

  * `INIT`: Requires `HELLO`
  * `NAME`: Login via predefined name list (up to 5 trials)
  * `READY`: Confirms readiness (`yes`)
  * `PARTNER`: Chooses a chat partner
  * `CHAT`: Bidirectional message relay
* Handles `SIGINT` for graceful shutdown
* Uses `setsockopt()` to avoid port reuse delay

### Client (`clipingup.c`)

* Connects to server using TCP
* Forks into two processes:

  * One for sending input from user
  * One for printing messages received from server
* Simple interface for chatting with chosen partner

## 🔧 How to Compile

```bash
gcc hw7.c -o server
gcc clipingup.c -o client
```

## 🚀 How to Run

Start server:

```bash
./server <port>
```

Start client:

```bash
./client <server_ip> <port>
```

## 🗂 Protocol Workflow

1. **Client connects**
2. **Server responds with welcome**
3. **Client must say `HELLO`**
4. **Login with valid name (max 5 attempts)**
5. **Confirm readiness**
6. **Choose a chat partner**
7. **Start chatting**

![image](https://github.com/user-attachments/assets/6cb1287e-761e-476e-afc3-2592715662bc)


## 📌 Notes

* Login names must exist in the login file (`login.txt`)
* Each client can chat with one other client at a time
* Server manages state transitions manually for control

## 📚 References

* UNIX Socket Programming
* `select()`, `fork()`, `SIGINT` handling in Linux
* C standard library (stdio, stdlib, string, unistd)

## 🔮 Future Improvements

* Support for file upload/download commands
* Add TLS encryption
* GUI client with ncurses or Qt
* Persistent message history
