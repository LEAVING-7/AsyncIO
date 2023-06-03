# AsyncIO: linux non-blocking network io based on AsyncTask
[![license][badge.license]][license]
[![language][badge.language]][language]

## Overview
* Tcp
  - async::TcpStream
  - async::TcpListener
* Tcp and TLS
  - async::TlsContext
  - async::TlsStream
  - async::TlsListener

## Usage
See [example/example_tcp_server.cpp](./examples/example_tcp_server.cpp) for a basic HTTP 200 server implementation

These are output from Apache ab.
```
# using InlineExecutor
$ ab -n 10000000 -c 1000 -k http://127.0.0.1:8080/
This is ApacheBench, Version 2.3 <$Revision: 1879490 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000000 requests
Completed 2000000 requests
Completed 3000000 requests
Completed 4000000 requests
Completed 5000000 requests
Completed 6000000 requests
Completed 7000000 requests
Completed 8000000 requests
Completed 9000000 requests
Completed 10000000 requests
Finished 10000000 requests


Server Software:
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        12 bytes

Concurrency Level:      1000
Time taken for tests:   304.138 seconds
Complete requests:      10000000
Failed requests:        0
Keep-Alive requests:    0
Total transferred:      510000000 bytes
HTML transferred:       120000000 bytes
Requests per second:    32879.83 [#/sec] (mean)
Time per request:       30.414 [ms] (mean)
Time per request:       0.030 [ms] (mean, across all concurrent requests)
Transfer rate:          1637.57 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   15   2.4     15      31
Processing:     3   16   2.6     16      37
Waiting:        0   11   1.8     11      32
Total:         16   30   1.5     30      55

Percentage of the requests served within a certain time (ms)
  50%     30
  66%     31
  75%     31
  80%     31
  90%     32
  95%     33
  98%     34
  99%     36
 100%     55 (longest request)
```

See [example/example_ssl_client.cpp](./examples/example_ssl_client.cpp) for a basic HTTPS request implementation

- Known issue: When using the `async::Runtime<async::MultiThreadExecutor>`, address sanitizer reports memory leaks in OpenSSL functions.

See [example/example_ssl_server.cpp](./examples/example_ssl_server.cpp) for a basic HTTPS 200 server

[badge.license]: https://img.shields.io/github/license/LEAVING-7/AsyncTask
[badge.language]: https://img.shields.io/badge/language-C%2B%2B20-yellow.svg

[language]: https://en.wikipedia.org/wiki/C%2B%2B20
[license]: https://en.wikipedia.org/wiki/Apache_License