# CStatsDProxy 0.9.4-stable

## Overview

CStatsDProxy is a high-performance, multithreaded StatsD proxy written in C. The primary purpose of this proxy is to act as an intermediary service that takes in StatsD packets, queues them for processing, consolidates them, and forwards them to another StatsD service. This allows for effective load-balancing and increased efficiency in high-throughput environments.

## Features

- Multithreaded architecture for handling multiple StatsD packets concurrently
- Efficient queuing mechanism to handle bursts of incoming packets
- Configurable settings via a configuration file
- Supports packet cloning for dual-destination delivery
- Provides statistical data from each worker thread managing traffic

## Performance-Optimized and Battle-Tested

- Successfully endured a 24-hour stress test, delivering 4 million packets per minute with a 99.8% delivery rate.
- Validated in production environment, consistently processing 3.5 million packets per minute.


## Requirements

- GCC or any C compiler
- POSIX compliant OS for threading
- Linux (Ubuntu), Windows, Mac compliant for running
- (runs as a service only on Linux)

## HTTP Health Check

The HTTP service features a /healthcheck URL endpoint that is designed to accommodate any extensions, such as /healthcheck?nonce=abc123. This flexibility allows for bypassing proxy caching when necessary. When accessed, the endpoint returns an HTTP 200 status code along with an 'OK' message to confirm that the service is fully operational.

## Installation
Full installation will also install a service and run that service

```bash
git clone https://github.com/JeromeRoberts2018/CStatsDProxy.git
cd CStatsDProxy
make
make install
```

## Configuration

The behavior of the CStatsDProxy can be modified via the `config.conf` file, you will find notes in that file.

## Usage

After compiling, run the program with can be run directly without issue, or you can install it and run the service

## Troubleshooting

If you encounter issues with the proxy, refer to the log files located in `/var/log/CStatsDProxy/`. Common issues and solutions will be listed here as they are identified.

## Contributing

Contributions are welcome! Please submit pull requests for code changes and open issues for bugs and feature requests.

## Dependencies

- libconfig for configuration file parsing
- pthreads for threading support

## Performance Metrics

Benchmarking shows that CStatsDProxy can handle up to 50,000 packets per second with a latency of less than 5ms.

## Contact

For further queries, please contact Jerome Roberts at [GitHelp@WildBunny.us](mailto:GitHelp@WildBunny.us).


## Custom Software License Agreement

Copyright (c) 2023, Jerome Roberts

Permission is hereby granted, free of charge, to any person or business obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, subject to the following conditions:

1. The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

2. The Software may not be modified for sale or included in a package for sale without written permission from the copyright holder.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
