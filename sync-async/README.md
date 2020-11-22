# Sync/Async

1. [server_v1.c](https://github.com/adwait1-G/Rust-C-Experiments/blob/main/sync-async/server_v1.c): Simple single request-response server. Serves one connection at a time. Uses blocking calls.
2. [server_v2.c](https://github.com/adwait1-G/Rust-C-Experiments/blob/main/sync-async/server_v2.c): Simple single request-response server. Serves multiple connections at a time by spawning new processes. Uses blocking calls.
3. [server_v3.c](https://github.com/adwait1-G/Rust-C-Experiments/blob/main/sync-async/server_v3.c): Simple single request-response server. Serves multiple connections by spawning new threads. Uses blocking calls.
4. [server_v4.c](https://github.com/adwait1-G/Rust-C-Experiments/blob/main/sync-async/server_v4.c): Single-threaded echo server using **select**.
5. [echo_server_v0.c](https://github.com/adwait1-G/Rust-C-Experiments/blob/main/sync-async/echo_server_v0.c): Echo server which serves one connection at a time. Uses blocking calls.
6. [echo_server_v1.c](https://github.com/adwait1-G/Rust-C-Experiments/blob/main/sync-async/echo_server_v1.c): Single-threaded echo server implemented using **select**.
7. [echo_server_v2.c](https://github.com/adwait1-G/Rust-C-Experiments/blob/main/sync-async/echo_server_v2.c): Single-threaded echo server implemented using the concept of polling. It sleeps, polls for events, processes if any and goes back to sleep. Doesn't use any event-notification facility like select, poll or epoll.
8. [echo_server_v3.c](https://github.com/adwait1-G/Rust-C-Experiments/blob/main/sync-async/echo_server_v3.c): Single-threaded echo server implemented using **poll**.