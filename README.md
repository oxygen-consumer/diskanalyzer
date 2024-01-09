# diskanalyzer

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

## Description

Analyze disk space inside the given path. This project is composed of a daemon and a CLI, the CLI sends requests to the daemon and the daemon processes them.
For features see [Usage](#usage)

## Table of Contents

- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Installation

Download the source code locally
```sh
git clone https://github.com/oxygen-consumer/diskanalyzer.git
```

Build and install
```sh
cd diskanalyzer
make && sudo make install
```

## Usage

First, you need to run the daemon by executing the following command
```
diskanalyzer
```

Then you can interact with it using the da command. The usage for da command is:
```
Options:
  -a, --add <path>                      Analyze directory
  -P, --priority <low/normal/high>      Set priority
  -S, --suspend <id>                    Suspend task
  -R, --resume <id>                     Resume task
  -r, --remove <id>                     Remove task
  -i, --info <id>                       Print task status
  -l, --list                            List all analysis tasks
  -p, --print <id>                      Print task analysis report
```

## License

This project is licensed under the [MIT License](LICENSE).
