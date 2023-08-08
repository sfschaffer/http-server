# HTTP Server

This program creates a HTTP server capable of handling GET, PUT, and APPEND requests while ensuring their atomicity and coherency. It is capable of using multiple threads, the amount of which is specified by the user.

In order to run it, simply make and then input ./httpserver -t threadCount -l logfile [port], where threadCount is an int specifying the amount of threads, logfile is a filename that will be used to log the actions of the server, and [port] is the port number

# Description

In order to maintain atomicity and coherency, I introduced a hash table that maps uri strings to mutex locks, making it so every time an operation is performed on a file, the lock corresponding to it is acquired, and when the operation is complete, that lock is released. In addition, PUT and APPEND now write to a tempfile, as this was the only way to ensure that GET operations would actually go through when a file has already started being written to.

# Bugs

This program(as of now) will segfault occasionally when using multiple cores. I have a hard time testing this, as my laptop is a dual core machine so it can be hard to reproduce on it.
